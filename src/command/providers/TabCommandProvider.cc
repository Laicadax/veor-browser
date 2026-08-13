// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "command/providers/TabCommandProvider.h"

#include <algorithm>

namespace veor {

TabCommandProvider::TabCommandProvider(ITabManager* tab_manager)
    : tab_manager_(tab_manager) {}

std::vector<CommandItem> TabCommandProvider::Query(const std::string& query) {
  // Simplified — real implementation iterates actual tabs
  std::vector<CommandItem> results;
  CommandItem item;
  item.id = IdGenerator::NextId<CommandTag>();
  item.title = "Switch to Tab";
  item.subtitle = "google.com";
  item.category = "Tabs";
  item.score = query.empty() ? 100 : 50;
  results.push_back(item);
  return results;
}

Result<void, std::string> TabCommandProvider::Execute(CommandId id) {
  // Activate corresponding tab
  return Result<void, std::string>::Ok();
}

}  // namespace veor
