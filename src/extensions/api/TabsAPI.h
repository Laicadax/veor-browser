// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "extensions/ExtensionAPI.h"
#include "base/memory/raw_ptr.h"
#include "tabs/ITabManager.h"

namespace veor {

class TabsAPI : public ExtensionAPI {
 public:
  explicit TabsAPI(ITabManager* tab_manager);
  ~TabsAPI() override = default;

  const char* GetNamespace() const override { return "tabs"; }
  Result<base::Value, std::string> Invoke(
      const std::string& method,
      const base::Value::List& args) override;
  std::string GetJSShim() const override;

 private:
  Result<base::Value, std::string> Create(const base::Value::List& args);
  Result<base::Value, std::string> Query(const base::Value::List& args);
  Result<base::Value, std::string> Update(const base::Value::List& args);
  Result<base::Value, std::string> Remove(const base::Value::List& args);
  Result<base::Value, std::string> Get(const base::Value::List& args);

  base::Value::Dict TabInfoToDict(const TabInfo& info);

  raw_ptr<ITabManager> tab_manager_;
};

}  // namespace veor
