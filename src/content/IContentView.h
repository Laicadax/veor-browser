// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>

#include "base/functional/callback.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// IContentView
// ─────────────────────────────────────────────────────────────────────────────
// Abstraction over the web content area. Wraps WebContents when integrated.
// For now: placeholder view that displays URL and loading state.
// ─────────────────────────────────────────────────────────────────────────────

class IContentView {
 public:
  virtual ~IContentView() = default;

  virtual void LoadUrl(const GURL& url) = 0;
  virtual void Reload() = 0;
  virtual void Stop() = 0;

  virtual GURL GetCurrentUrl() const = 0;
  virtual bool IsLoading() const = 0;

  virtual void SetOnTitleChanged(base::RepeatingCallback<void(const std::string&)> cb) = 0;
  virtual void SetOnUrlChanged(base::RepeatingCallback<void(const GURL&)> cb) = 0;
  virtual void SetOnLoadingStateChanged(base::RepeatingCallback<void(bool)> cb) = 0;
};

}  // namespace veor
