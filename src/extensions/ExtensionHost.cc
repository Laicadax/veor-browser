// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/ExtensionHost.h"

#include "core/logging/VeorLogger.h"
#include "extensions/api/BookmarksAPI.h"
#include "extensions/api/StorageAPI.h"
#include "extensions/api/TabsAPI.h"

namespace veor {

ExtensionHost::ExtensionHost(ITabManager* tab_manager,
                             IBookmarkStore* bookmark_store,
                             ISettingsProvider* settings)
    : tab_manager_(tab_manager),
      bookmark_store_(bookmark_store),
      settings_(settings) {}

ExtensionHost::~ExtensionHost() = default;

void ExtensionHost::Initialize() {
  if (tab_manager_)
    RegisterAPI(std::make_unique<TabsAPI>(tab_manager_));
  if (bookmark_store_)
    RegisterAPI(std::make_unique<BookmarksAPI>(bookmark_store_));
  if (settings_)
    RegisterAPI(std::make_unique<StorageAPI>(settings_));

  VEOR_LOGI(LogCategory::kContent,
            "ExtensionHost initialized with " + std::to_string(apis_.size()) +
                " APIs");
}

void ExtensionHost::RegisterAPI(std::unique_ptr<ExtensionAPI> api) {
  apis_[api->GetNamespace()] = std::move(api);
}

Result<base::Value, std::string> ExtensionHost::HandleCall(
    const std::string& namespace_and_method,
    const base::Value::List& args) {
  size_t dot = namespace_and_method.find('.');
  if (dot == std::string::npos) {
    return Result<base::Value, std::string>::Err("Invalid API call format: " + namespace_and_method);
  }

  std::string ns = namespace_and_method.substr(0, dot);
  std::string method = namespace_and_method.substr(dot + 1);

  auto it = apis_.find(ns);
  if (it == apis_.end()) {
    return Result<base::Value, std::string>::Err("Unknown API namespace: " + ns);
  }

  return it->second->Invoke(method, args);
}

std::string ExtensionHost::GetAllJSShims() const {
  std::string shims =
      "(function() {"
      "  if (typeof chrome === 'undefined') window.chrome = {};"
      "  window.__veor_api_call = function(ns, method, args) {"
      "    return new Promise((resolve, reject) => {"
      "      if (typeof __veor_native_api_call !== 'undefined') {"
      "        __veor_native_api_call(ns + '.' + method, JSON.stringify(args))"
      "          .then(r => resolve(JSON.parse(r)))"
      "          .catch(e => reject(e));"
      "      } else {"
      "        reject(new Error('VEOR native API not available'));"
      "      }"
      "    });"
      "  };";

  for (const auto& [ns, api] : apis_) {
    shims += api->GetJSShim();
  }

  shims += "})();";
  return shims;
}

std::vector<std::string> ExtensionHost::GetNamespaces() const {
  std::vector<std::string> result;
  result.reserve(apis_.size());
  for (const auto& [ns, api] : apis_) {
    result.push_back(ns);
  }
  return result;
}

}  // namespace veor
