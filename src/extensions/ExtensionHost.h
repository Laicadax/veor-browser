// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <set>
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

  // Grant an extension the permissions declared in its manifest. Calls from
  // an extension that has not been granted the permission matching the API
  // namespace are rejected.
  void SetExtensionPermissions(const std::string& extension_id,
                               const std::vector<std::string>& permissions);
  void RevokeExtension(const std::string& extension_id);

  // Handle a call from the renderer process.
  // extension_id: the caller's extension id, resolved by the browser process
  //   from the requesting frame. Never accept this value from the renderer.
  // namespace_and_method: "tabs.create", "bookmarks.getTree", etc.
  Result<base::Value, std::string> HandleCall(
      const std::string& extension_id,
      const std::string& namespace_and_method,
      const base::Value::List& args);

  // Get all JS shims concatenated for injection. These bindings expose
  // privileged browser APIs and must only be injected into extension
  // contexts, never into ordinary web frames.
  std::string GetAllJSShims() const;

  // Get list of supported API namespaces.
  std::vector<std::string> GetNamespaces() const;

 private:
  void RegisterAPI(std::unique_ptr<ExtensionAPI> api);

  ITabManager* tab_manager_;
  IBookmarkStore* bookmark_store_;
  ISettingsProvider* settings_;

  std::unordered_map<std::string, std::unique_ptr<ExtensionAPI>> apis_;
  std::unordered_map<std::string, std::set<std::string>> granted_permissions_;
};

}  // namespace veor
