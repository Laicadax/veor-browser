// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "extensions/ExtensionAPI.h"
#include "base/memory/raw_ptr.h"
#include "settings/ISettingsProvider.h"

namespace veor {

class StorageAPI : public ExtensionAPI {
 public:
  explicit StorageAPI(ISettingsProvider* settings);
  ~StorageAPI() override = default;

  const char* GetNamespace() const override { return "storage"; }
  Result<base::Value, std::string> Invoke(
      const std::string& method,
      const base::Value::List& args) override;
  std::string GetJSShim() const override;

 private:
  Result<base::Value, std::string> Get(const base::Value::List& args);
  Result<base::Value, std::string> Set(const base::Value::List& args);
  Result<base::Value, std::string> Remove(const base::Value::List& args);
  Result<base::Value, std::string> Clear(const base::Value::List& args);

  raw_ptr<ISettingsProvider> settings_;
  static constexpr char kStoragePrefix[] = "ext.storage.";
};

}  // namespace veor
