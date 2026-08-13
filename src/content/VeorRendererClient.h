// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/renderer/content_renderer_client.h"

namespace veor {

// VeorRendererClient hooks into the renderer process lifecycle.
// Used to inject custom JavaScript, CSS, or modify rendering behavior
// before any frames are created.
class VeorRendererClient : public content::ContentRendererClient {
 public:
  VeorRendererClient();
  ~VeorRendererClient() override;

  // content::ContentRendererClient
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderViewCreated(content::RenderView* render_view) override;

 private:
  void InjectVeorStyles(content::RenderFrame* frame);
};

}  // namespace veor
