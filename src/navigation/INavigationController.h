// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// INavigationController
// ─────────────────────────────────────────────────────────────────────────────
// Per-tab back/forward stack. Not global. Each tab owns its history.
// ─────────────────────────────────────────────────────────────────────────────

struct NavigationEntry {
  GURL url;
  std::string title;
  base::Time timestamp;
};

class INavigationController {
 public:
  virtual ~INavigationController() = default;

  virtual void PushEntry(const NavigationEntry& entry) = 0;
  virtual bool CanGoBack() const = 0;
  virtual bool CanGoForward() const = 0;
  virtual GURL GoBack() = 0;      // Returns previous URL, or empty if none
  virtual GURL GoForward() = 0;   // Returns next URL, or empty if none
  virtual GURL GetCurrentUrl() const = 0;

  virtual void SetOnUrlChanged(base::RepeatingCallback<void(const GURL&)> cb) = 0;
};

}  // namespace veor
