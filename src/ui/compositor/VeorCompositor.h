// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/memory/weak_ptr.h"
#include "ui/compositor/compositor.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// VeorCompositor
// ─────────────────────────────────────────────────────────────────────────────
// Hardware-accelerated compositor for VEOR chrome UI.
// Wraps ui::Compositor with VEOR-specific features:
//   - GPU rasterization toggle
//   - Tab thumbnail capture
//   - Frame rate limiting
//   - Memory pressure response

class VeorCompositor : public ui::Compositor {
 public:
  explicit VeorCompositor(ui::CompositorDelegate* delegate);
  ~VeorCompositor() override;

  // GPU rasterization control
  void SetGpuRasterization(bool enabled);
  bool IsGpuRasterizationEnabled() const;

  // Tab thumbnail capture — captures a layer tree into a bitmap
  // asynchronously. Callback receives the captured SkBitmap.
  void CaptureThumbnail(ui::Layer* source_layer,
                        const gfx::Size& thumbnail_size,
                        base::OnceCallback<void(const SkBitmap&)> callback);

  // Frame rate limiting for power saving
  void SetFrameRateLimit(int fps);  // 0 = unlimited
  int GetFrameRateLimit() const;

  // Called when system reports memory pressure
  void OnMemoryPressure();

  // Returns compositor statistics for debugging
  struct Stats {
    int frame_count = 0;
    int dropped_frame_count = 0;
    float average_frame_time_ms = 0.0f;
    bool gpu_rasterization = false;
    size_t texture_memory_bytes = 0;
  };
  Stats GetStats() const;

 private:
  void OnCompositorRequestLayout();
  void UpdateFrameRate();

  bool gpu_rasterization_ = true;
  int frame_rate_limit_ = 0;
  Stats stats_;

  base::WeakPtrFactory<VeorCompositor> weak_factory_{this};
};

}  // namespace veor
