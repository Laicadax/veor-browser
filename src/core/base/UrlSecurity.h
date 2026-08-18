// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "url/gurl.h"
#include "url/url_constants.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// URL scheme policy for untrusted navigation requests
// ─────────────────────────────────────────────────────────────────────────────
// Untrusted sources are the omnibox (pasted text), extensions, and anything
// else driven by renderer input. These schemes either execute script in the
// context of the document that requested the navigation or let the requester
// pick an origin of its choosing.

inline bool IsDangerousNavigationScheme(const GURL& url) {
  static constexpr const char* kBlockedSchemes[] = {
      "javascript", "data", "vbscript", "blob", "filesystem"};
  for (const char* scheme : kBlockedSchemes) {
    if (url.SchemeIs(scheme))
      return true;
  }
  return false;
}

// URLs an extension or a renderer may navigate a tab to.
inline bool IsWebNavigableUrl(const GURL& url) {
  if (!url.is_valid() || IsDangerousNavigationScheme(url))
    return false;
  return url.SchemeIs(url::kHttpScheme) || url.SchemeIs(url::kHttpsScheme) ||
         url.IsAboutBlank();
}

}  // namespace veor
