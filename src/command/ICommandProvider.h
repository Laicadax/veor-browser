// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// CommandItem — Single entry in the command palette
// ─────────────────────────────────────────────────────────────────────────────

struct CommandItem {
  CommandId id;
  std::string title;
  std::string subtitle;     // Context, e.g. "Tab: google.com"
  std::string category;     // "Tabs", "History", "Settings", etc.
  int score = 0;            // Relevance for sorting
};

// ─────────────────────────────────────────────────────────────────────────────
// ICommandProvider — Source of commands for the palette
// ─────────────────────────────────────────────────────────────────────────────

class ICommandProvider {
 public:
  virtual ~ICommandProvider() = default;

  // Returns all items matching the query. Empty query = all items.
  virtual std::vector<CommandItem> Query(const std::string& query) = 0;

  // Execute the selected command
  virtual Result<void, std::string> Execute(CommandId id) = 0;

  // Provider name for debugging
  virtual std::string GetName() const = 0;
};

}  // namespace veor
