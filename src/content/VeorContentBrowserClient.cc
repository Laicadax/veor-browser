// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/VeorContentBrowserClient.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "content/public/browser/browser_context.h"

#include "content/VeorBrowserContext.h"
#include "content/VeorBrowserMainParts.h"
#include "core/logging/VeorLogger.h"
#include "devtools/VeorDevToolsDelegate.h"
#include "navigation/VeorNavigationThrottle.h"
#include "safe_browsing/SafeBrowsingService.h"

namespace veor {

VeorContentBrowserClient::VeorContentBrowserClient() = default;
VeorContentBrowserClient::~VeorContentBrowserClient() = default;

content::BrowserContext* VeorContentBrowserClient::CreateBrowserContext() {
  browser_context_ = std::make_unique<VeorBrowserContext>();
  VEOR_LOGI(LogCategory::kCore, "Browser context created");
  return browser_context_.get();
}

VeorBrowserContext* VeorContentBrowserClient::GetBrowserContext() const {
  return browser_context_.get();
}

content::BrowserMainParts* VeorContentBrowserClient::CreateBrowserMainParts() {
  return new VeorBrowserMainParts();
}

content::DevToolsManagerDelegate*
VeorContentBrowserClient::GetDevToolsManagerDelegate() {
  return new VeorDevToolsDelegate();
}

content::BrowserPluginGuestManager*
VeorContentBrowserClient::GetGuestViewManager(
    content::BrowserContext* context) {
  return nullptr;
}

base::FilePath VeorContentBrowserClient::GetDefaultDownloadDirectory() {
  base::FilePath path;
  base::PathService::Get(base::DIR_HOME, &path);
  return path.AppendASCII("Downloads");
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
VeorContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;

  if (browser_context_) {
    SafeBrowsingService* sb = browser_context_->GetSafeBrowsingService();
    if (sb) {
      throttles.push_back(
          std::make_unique<VeorNavigationThrottle>(handle, sb));
    }
  }

  return throttles;
}

}  // namespace veor
