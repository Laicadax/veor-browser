// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/api/StorageAPI.h"

namespace veor {

StorageAPI::StorageAPI(ISettingsProvider* settings) : settings_(settings) {}

Result<base::Value, std::string> StorageAPI::Invoke(
    const std::string& method,
    const base::Value::List& args) {
  if (method == "get") return Get(args);
  if (method == "set") return Set(args);
  if (method == "remove") return Remove(args);
  if (method == "clear") return Clear(args);
  return Err("Unknown method: storage." + method);
}

Result<base::Value, std::string> StorageAPI::Get(const base::Value::List& args) {
  if (args.empty()) return Err("storage.get requires keys");
  base::Value::Dict result;

  if (args[0].is_string()) {
    std::string key = std::string(kStoragePrefix) + args[0].GetString();
    auto val = settings_->GetValue(key);
    if (val.IsOk()) {
      result.Set(args[0].GetString(), val.Value().ToBaseValue());
    }
  } else if (args[0].is_list()) {
    for (const auto& item : args[0].GetList()) {
      if (!item.is_string()) continue;
      std::string key = std::string(kStoragePrefix) + item.GetString();
      auto val = settings_->GetValue(key);
      if (val.IsOk()) {
        result.Set(item.GetString(), val.Value().ToBaseValue());
      }
    }
  } else if (args[0].is_dict()) {
    for (const auto [key, default_val] : args[0].GetDict()) {
      std::string full_key = std::string(kStoragePrefix) + key;
      auto val = settings_->GetValue(full_key);
      if (val.IsOk()) {
        result.Set(key, val.Value().ToBaseValue());
      } else {
        result.Set(key, default_val.Clone());
      }
    }
  }
  return Ok(base::Value(std::move(result)));
}

Result<base::Value, std::string> StorageAPI::Set(const base::Value::List& args) {
  if (args.empty() || !args[0].is_dict())
    return Err("storage.set requires items object");
  for (const auto [key, val] : args[0].GetDict()) {
    std::string full_key = std::string(kStoragePrefix) + key;
    settings_->SetValue(full_key, SettingValue::FromBaseValue(val));
  }
  return Ok(base::Value());
}

Result<base::Value, std::string> StorageAPI::Remove(const base::Value::List& args) {
  if (args.empty()) return Err("storage.remove requires keys");
  if (args[0].is_string()) {
    settings_->ResetToDefault(std::string(kStoragePrefix) + args[0].GetString());
  } else if (args[0].is_list()) {
    for (const auto& item : args[0].GetList()) {
      if (item.is_string()) {
        settings_->ResetToDefault(std::string(kStoragePrefix) + item.GetString());
      }
    }
  }
  return Ok(base::Value());
}

Result<base::Value, std::string> StorageAPI::Clear(const base::Value::List& args) {
  // TODO: iterate all ext.storage.* keys and reset
  return Ok(base::Value());
}

std::string StorageAPI::GetJSShim() const {
  return R"js(
(function() {
  const ns = 'storage';
  const makeArea = (area) => ({
    get: (keys) => __veor_api_call(ns, 'get', [keys]),
    set: (items) => __veor_api_call(ns, 'set', [items]),
    remove: (keys) => __veor_api_call(ns, 'remove', [keys]),
    clear: () => __veor_api_call(ns, 'clear', []),
  });
  chrome.storage = {
    local: makeArea('local'),
    sync: makeArea('sync'),
    onChanged: { addListener: () => {}, removeListener: () => {} },
  };
})();
)js";
}

}  // namespace veor
