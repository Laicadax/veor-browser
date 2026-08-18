// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "command/ICommandProvider.h"
#include "base/memory/raw_ptr.h"
#include "history/IHistoryStore.h"

namespace veor {

class HistoryCommandProvider : public ICommandProvider {
 public:
  explicit HistoryCommandProvider(IHistoryStore* history);

  std::vector<CommandItem> Query(const std::string& query) override;
  Result<void, std::string> Execute(CommandId id) override;
  std::string GetName() const override { return "History"; }

 private:
  raw_ptr<IHistoryStore> history_;
};

}  // namespace veor
