// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/VeorCompositor.h"

#include "base/time/time.h"
#include "cc/trees/layer_tree_host.h"
#include "core/logging/VeorLogger.h"
#include "ui/compositor/layer.h"

namespace veor {

VeorCompositor::VeorCompositor(ui::CompositorDelegate* delegate)
    : ui::Compositor(delegate, /*context_factory=*/nullptr) {
  VEOR_LOGI(LogCategory::kUI, "VeorCompositor initialized");
}

VeorCompositor::~VeorCompositor() = default;

void VeorCompositor::SetGpuRasterization(bool enabled) {
  gpu_rasterization_ = enabled;

  auto* host = layer_tree_host();
  if (host) {
    host->SetHasGpuRasterizationTrigger(enabled);
    host->SetContentUsesWideColorGamut(false);
  }

  VEOR_LOGI(LogCategory::kUI,
            "GPU rasterization " + std::string(enabled ? "enabled" : "disabled"));
}

bool VeorCompositor::IsGpuRasterizationEnabled() const {
  return gpu_rasterization_;
}

void VeorCompositor::CaptureThumbnail(
    ui::Layer* source_layer,
    const gfx::Size& thumbnail_size,
    base::OnceCallback<void(const SkBitmap&)> callback) {
  if (!source_layer) {
    std::move(callback).Run(SkBitmap());
    return;
  }

  // Schedule a readback from the compositor
  RequestNewCompositorFrame();

  // Post a delayed task to capture after the frame is rendered
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<VeorCompositor> compositor,
             ui::Layer* layer,
             gfx::Size size,
             base::OnceCallback<void(const SkBitmap&)> cb) {
            if (!compositor || !layer) {
              std::move(cb).Run(SkBitmap());
              return;
            }

            SkBitmap bitmap;
            bitmap.allocN32Pixels(size.width(), size.height());
            bitmap.eraseColor(SK_ColorTRANSPARENT);

            // TODO: Use CopyOutputRequest for proper GPU readback
            // For now, return a placeholder bitmap
            std::move(cb).Run(bitmap);
          },
          weak_factory_.GetWeakPtr(),
          base::Unretained(source_layer),
          thumbnail_size,
          std::move(callback)),
      base::Milliseconds(50));
}

void VeorCompositor::SetFrameRateLimit(int fps) {
  frame_rate_limit_ = std::max(0, fps);
  UpdateFrameRate();

  VEOR_LOGI(LogCategory::kUI,
            "Frame rate limit set to " +
                std::string(fps == 0 ? "unlimited" : std::to_string(fps) + " FPS"));
}

int VeorCompositor::GetFrameRateLimit() const {
  return frame_rate_limit_;
}

void VeorCompositor::UpdateFrameRate() {
  auto* host = layer_tree_host();
  if (!host)
    return;

  if (frame_rate_limit_ > 0) {
    base::TimeDelta interval = base::Seconds(1) / frame_rate_limit_;
    host->SetDeadlinePolicy(
        cc::LayerTreeHost::DeadlinePolicy::UseSpecifiedDeadline(interval));
  } else {
    host->SetDeadlinePolicy(cc::LayerTreeHost::DeadlinePolicy::UseDefaultDeadline());
  }
}

void VeorCompositor::OnMemoryPressure() {
  VEOR_LOGW(LogCategory::kUI, "Compositor responding to memory pressure");

  // Drop GPU resources
  auto* host = layer_tree_host();
  if (host) {
    host->SetHasGpuRasterizationTrigger(false);
    host->ReleaseLayerTreeFrameSink();
  }

  // Reduce frame rate to save power
  SetFrameRateLimit(30);
}

VeorCompositor::Stats VeorCompositor::GetStats() const {
  Stats s = stats_;
  s.gpu_rasterization = gpu_rasterization_;

  auto* host = layer_tree_host();
  if (host) {
    // Approximate texture memory from layer count
    s.texture_memory_bytes = 0;  // Would need viz::FrameSinkManager for real value
  }

  return s;
}

}  // namespace veor
