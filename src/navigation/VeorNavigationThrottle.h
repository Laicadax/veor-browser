// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/navigation_throttle.h"
#include "net/ssl/ssl_info.h"

namespace veor {

class SafeBrowsingService;

// ─────────────────────────────────────────────────────────────────────────────
// VeorNavigationThrottle
// ─────────────────────────────────────────────────────────────────────────────
// Intercepts all navigations (window.location, <a href>, iframe, form submit)
// before they commit. Checks URL against Safe Browsing database.
//
// Unlike NetworkDelegateImpl which only sees HTTP requests, this catches
// all navigation types including JavaScript-initiated redirects.
//
// On threat detection: cancels navigation and shows blocked page.
// ─────────────────────────────────────────────────────────────────────────────

class VeorNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit VeorNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      SafeBrowsingService* safe_browsing);
  ~VeorNavigationThrottle() override;

  // content::NavigationThrottle
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  ThrottleCheckResult WillProcessResponse() override;
  ThrottleCheckResult WillFailRequest() override;

  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult CheckUrl(const GURL& url);
  ThrottleCheckResult CheckSSL(const net::SSLInfo& ssl_info, const GURL& url);

  SafeBrowsingService* safe_browsing_ = nullptr;
};

}  // namespace veor