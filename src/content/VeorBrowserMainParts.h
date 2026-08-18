// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
class Workspace;

#include <memory>
#include "base/memory/raw_ptr.h"

#include "content/public/browser/browser_main_parts.h"

namespace veor {

class IThemeProvider;
class IWorkspaceManager;
class BrowserShell;

// ─────────────────────────────────────────────────────────────────────────────
// VeorBrowserMainParts
// ─────────────────────────────────────────────────────────────────────────────
// Creates the BrowserShell window before the message loop starts.
// Owns the theme provider and workspace manager for the browser process.
// ─────────────────────────────────────────────────────────────────────────────

class VeorBrowserMainParts : public content::BrowserMainParts {
 public:
  VeorBrowserMainParts();
  ~VeorBrowserMainParts() override;

  // content::BrowserMainParts
  int PreEarlyInitialization() override;
  void PostEarlyInitialization() override;
  void PreMainMessageLoopStart() override;
  void PostMainMessageLoopStart() override;
  int PreCreateThreads() override;
  void PostCreateThreads() override;
  int PreMainMessageLoopRun() override;
  void WillRunMainMessageLoop(
      std::unique_ptr<base::RunLoop>& run_loop) override;
  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;

 private:
  std::unique_ptr<IThemeProvider> theme_;
  raw_ptr<IWorkspaceManager> workspace_manager_= nullptr;
  raw_ptr<BrowserShell> shell_= nullptr;
  std::unique_ptr<views::Widget> widget_;
};

}  // namespace veor
