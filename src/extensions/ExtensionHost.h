// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "base/memory/raw_ptr.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "base/values.h"
#include "core/base/VeorResult.h"
#include "extensions/ExtensionAPI.h"

namespace veor {

class ITabManager;
class IBookmarkStore;
class ISettingsProvider;

// ─────────────────────────────────────────────────────────────────────────────
// ExtensionHost
// ─────────────────────────────────────────────────────────────────────────────
// Manages chrome.* API bindings for content scripts and extensions.
// Registered APIs:
//   - chrome.tabs (create, query, update, remove)
//   - chrome.bookmarks (create, getTree, search, remove)
//   - chrome.storage (local.get, local.set, local.remove, local.clear)

class ExtensionHost {
 public:
  ExtensionHost(ITabManager* tab_manager,
                IBookmarkStore* bookmark_store,
                ISettingsProvider* settings);
  ~ExtensionHost();

  // Register all built-in APIs.
  void Initialize();

  // Handle a call from the renderer process.
  // namespace_and_method: "tabs.create", "bookmarks.getTree", etc.
  Result<base::Value, std::string> HandleCall(
      const std::string& namespace_and_method,
      const base::Value::List& args);

  // Get all JS shims concatenated for injection.
  std::string GetAllJSShims() const;

  // Get list of supported API namespaces.
  std::vector<std::string> GetNamespaces() const;

 private:
  void RegisterAPI(std::unique_ptr<ExtensionAPI> api);

  raw_ptr<ITabManager> tab_manager_;
  raw_ptr<IBookmarkStore> bookmark_store_;
  raw_ptr<ISettingsProvider> settings_;

  std::unordered_map<std::string, std::unique_ptr<ExtensionAPI>> apis_;
};

}  // namespace veor
