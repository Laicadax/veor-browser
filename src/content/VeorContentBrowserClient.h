// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "base/files/file_path.h"

#include "content/public/browser/content_browser_client.h"

namespace content {
class BrowserMainParts;
}  // namespace content

namespace veor {

class VeorBrowserContext;

// ─────────────────────────────────────────────────────────────────────────────
// VeorContentBrowserClient
// ─────────────────────────────────────────────────────────────────────────────
// Customizes browser-process behavior for VEOR. Minimal implementation:
//   - Creates VeorBrowserContext (profile)
//   - Provides DevTools delegate (stub, Stage 5)
//   - No GuestView, no WebRTC, no Push messaging
// ─────────────────────────────────────────────────────────────────────────────

class VeorContentBrowserClient : public content::ContentBrowserClient {
 public:
  VeorContentBrowserClient();
  ~VeorContentBrowserClient() override;

  // Browser context (profile) factory
  content::BrowserContext* CreateBrowserContext() override;
  VeorBrowserContext* GetBrowserContext() const;

  // Browser main parts — creates BrowserShell window
  content::BrowserMainParts* CreateBrowserMainParts() override;

  // DevTools — stub, implemented in Stage 5
  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  // No guest views
  content::BrowserPluginGuestManager* GetGuestViewManager(
      content::BrowserContext* context) override;

  // Download path — default to profile directory
  base::FilePath GetDefaultDownloadDirectory() override;

  // Safe Browsing: intercept all navigations
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(content::NavigationHandle* handle) override;

 private:
  std::unique_ptr<VeorBrowserContext> browser_context_;
};

}  // namespace veor
