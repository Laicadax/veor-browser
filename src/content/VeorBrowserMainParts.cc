// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/VeorBrowserMainParts.h"

#include "base/files/file_path.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "ui/views/widget/widget.h"

#include "content/VeorBrowserContext.h"
#include "content/VeorContentBrowserClient.h"
#include "core/logging/VeorLogger.h"
#include "ui/shell/BrowserShell.h"
#include "ui/theme/ThemeProviderImpl.h"
#include "workspace/IWorkspaceManager.h"
#include "workspace/WorkspaceManager.h"

namespace veor {

VeorBrowserMainParts::VeorBrowserMainParts() = default;
VeorBrowserMainParts::~VeorBrowserMainParts() = default;

int VeorBrowserMainParts::PreEarlyInitialization() {
  return 0;
}

void VeorBrowserMainParts::PostEarlyInitialization() {}

void VeorBrowserMainParts::PreMainMessageLoopStart() {}

void VeorBrowserMainParts::PostMainMessageLoopStart() {}

int VeorBrowserMainParts::PreCreateThreads() {
  return 0;
}

void VeorBrowserMainParts::PostCreateThreads() {}

int VeorBrowserMainParts::PreMainMessageLoopRun() {
  auto* client = static_cast<VeorContentBrowserClient*>(
      content::GetContentClient()->browser());
  DCHECK(client);

  auto* browser_context = client->GetBrowserContext();
  if (!browser_context) {
    browser_context = static_cast<VeorBrowserContext*>(client->CreateBrowserContext());
  }
  DCHECK(browser_context);

  IWorkspaceManager* workspace_manager = GetWorkspaceManager();

  // Initialize session persistence and restore workspaces
  base::FilePath profile_path = browser_context->GetPath();
  auto* ws_manager = static_cast<WorkspaceManager*>(workspace_manager);
  ws_manager->InitializePersistence(
      profile_path.Append(FILE_PATH_LITERAL("sessions.db")));
  ws_manager->LoadWorkspaces();

  theme_ = std::make_unique<ThemeProviderImpl>();

  auto* safe_browsing = browser_context->GetSafeBrowsingService();
  auto* shell = new BrowserShell(
      std::move(theme_), workspace_manager, browser_context, safe_browsing);
  shell->SetOwnedByWidget(true);

  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW);
  params.bounds = gfx::Rect(100, 100, 1280, 800);
  params.name = "VEOR";
  params.delegate = shell;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.remove_standard_frame = true;

  widget_ = std::make_unique<views::Widget>();
  widget_->Init(std::move(params));
  widget_->Show();

  shell_ = shell;

  VEOR_LOGI(LogCategory::kUI, "VEOR BrowserShell initialized");
  return 0;
}

void VeorBrowserMainParts::WillRunMainMessageLoop(
    std::unique_ptr<base::RunLoop>& run_loop) {}

void VeorBrowserMainParts::PostMainMessageLoopRun() {
  widget_.reset();
  VEOR_LOGI(LogCategory::kUI, "VEOR Browser shutting down");
}

void VeorBrowserMainParts::PostDestroyThreads() {}

}  // namespace veor
