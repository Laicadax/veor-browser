// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "content/public/app/content_main_delegate.h"

namespace veor {

class VeorContentBrowserClient;
class VeorRendererClient;

// ─────────────────────────────────────────────────────────────────────────────
// VeorMainDelegate
// ─────────────────────────────────────────────────────────────────────────────
// Entry point for the Chromium Content API. Replaces the manual
// base::SingleThreadTaskExecutor + views::Widget initialization in
// browser_main.cc with the full Content process model.
//
// Responsibilities:
//   - Create ContentBrowserClient
//   - Create ContentRendererClient
//   - Route process types (browser, renderer, gpu, utility)
//   - Initialize logging and paths before sandbox
// ─────────────────────────────────────────────────────────────────────────────

class VeorMainDelegate : public content::ContentMainDelegate {
 public:
  VeorMainDelegate();
  ~VeorMainDelegate() override;

  // content::ContentMainDelegate
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  void SandboxInitialized(const std::string& process_type) override;
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
  void ProcessExiting(const std::string& process_type) override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;

 private:
  std::unique_ptr<VeorContentBrowserClient> browser_client_;
  std::unique_ptr<VeorRendererClient> renderer_client_;
};

}  // namespace veor
