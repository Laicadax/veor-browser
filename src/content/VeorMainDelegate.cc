// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/VeorMainDelegate.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"

#include "content/VeorContentBrowserClient.h"
#include "content/VeorRendererClient.h"
#include "core/logging/VeorLogger.h"
#include "sandbox/VeorSandbox.h"

namespace veor {

VeorMainDelegate::VeorMainDelegate() = default;
VeorMainDelegate::~VeorMainDelegate() = default;

bool VeorMainDelegate::BasicStartupComplete(int* exit_code) {
  logging::LoggingSettings log_settings;
  log_settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(log_settings);

  VEOR_LOGI(LogCategory::kCore, "VEOR Content API starting");
  return false;  // Continue startup
}

void VeorMainDelegate::PreSandboxStartup() {
  // Initialize paths before sandbox locks us out of filesystem
  base::FilePath path;
  base::PathService::Get(base::DIR_EXE, &path);

  // Initialize platform sandbox
  if (!veor::InitializeSandbox()) {
    VEOR_LOGE(LogCategory::kCore, "Sandbox init failed — exiting");
    // Fatal: without sandbox renderer processes are insecure
  }
}

void VeorMainDelegate::SandboxInitialized(const std::string& process_type) {
  // No-op for MVP. In production: verify sandbox status, log violations.
}

int VeorMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (process_type.empty()) {
    // Browser process — run the main browser loop
    VEOR_LOGI(LogCategory::kCore, "Browser process started");
    return content::ContentMainDelegate::RunProcess(process_type,
                                                      main_function_params);
  }

  // Child processes (renderer, gpu, utility) use default ContentMain
  return -1;  // Return -1 to let ContentMain handle child processes
}

void VeorMainDelegate::ProcessExiting(const std::string& process_type) {
  VEOR_LOGI(LogCategory::kCore,
            "Process exiting: " + (process_type.empty() ? "browser" : process_type));
}

content::ContentBrowserClient* VeorMainDelegate::CreateContentBrowserClient() {
  browser_client_ = std::make_unique<VeorContentBrowserClient>();
  return browser_client_.get();
}

content::ContentRendererClient* VeorMainDelegate::CreateContentRendererClient() {
  renderer_client_ = std::make_unique<VeorRendererClient>();
  return renderer_client_.get();
}

}  // namespace veor
