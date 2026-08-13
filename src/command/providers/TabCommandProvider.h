// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "command/ICommandProvider.h"
#include "tabs/ITabManager.h"

namespace veor {

class TabCommandProvider : public ICommandProvider {
 public:
  explicit TabCommandProvider(ITabManager* tab_manager);

  std::vector<CommandItem> Query(const std::string& query) override;
  Result<void, std::string> Execute(CommandId id) override;
  std::string GetName() const override { return "Tabs"; }

 private:
  ITabManager* tab_manager_;
};

}  // namespace veor
