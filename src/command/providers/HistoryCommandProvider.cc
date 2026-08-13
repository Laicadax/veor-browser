// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "command/providers/HistoryCommandProvider.h"

#include "core/base/VeorId.h"

namespace veor {

HistoryCommandProvider::HistoryCommandProvider(IHistoryStore* history)
    : history_(history) {}

std::vector<CommandItem> HistoryCommandProvider::Query(const std::string& query) {
  std::vector<CommandItem> results;
  if (!query.empty()) {
    CommandItem item;
    item.id = IdGenerator::NextId<CommandTag>();
    item.title = "Open History: " + query;
    item.subtitle = "history";
    item.category = "History";
    item.score = 80;
    results.push_back(item);
  }
  return results;
}

Result<void, std::string> HistoryCommandProvider::Execute(CommandId id) {
  return Result<void, std::string>::Ok();
}

}  // namespace veor
