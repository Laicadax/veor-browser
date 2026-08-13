// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "base/values.h"
#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ExtensionAPI
// ─────────────────────────────────────────────────────────────────────────────
// Base class for chrome.* API implementations injected into the renderer.
// Each API namespace (tabs, bookmarks, storage) derives from this.

class ExtensionAPI {
 public:
  virtual ~ExtensionAPI() = default;

  // API namespace: "tabs", "bookmarks", "storage", etc.
  virtual const char* GetNamespace() const = 0;

  // Handle a method call from the renderer.
  // method: e.g. "create", "query", "get", "set"
  // args: JSON array of arguments
  // Returns JSON value or error string.
  virtual Result<base::Value, std::string> Invoke(
      const std::string& method,
      const base::Value::List& args) = 0;

  // Returns the JavaScript binding injected into each renderer frame.
  // This creates the chrome.<namespace> object with async promise wrappers.
  virtual std::string GetJSShim() const = 0;
};

}  // namespace veor
