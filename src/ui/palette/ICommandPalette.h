// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>
#include <vector>

#include "base/functional/callback.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ICommandPalette
// ─────────────────────────────────────────────────────────────────────────────
// Not a modal dialog. An overlay that emerges from darkness.
// Appears below the title bar, centered horizontally.
// No categories. No shortcut hints. Just a stream of results.
// ─────────────────────────────────────────────────────────────────────────────

struct PaletteItem {
  std::string title;
  std::string subtitle;   // URL, path, or context
  std::string type;       // "tab", "history", "nav", "command"
};

class ICommandPalette {
 public:
  virtual ~ICommandPalette() = default;

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() const = 0;

  virtual void SetQuery(const std::string& query) = 0;
  virtual std::string GetQuery() const = 0;
  virtual void SetResults(const std::vector<PaletteItem>& items) = 0;
  virtual void SetSelectedIndex(int index) = 0;

  virtual void SetOnItemSelected(base::RepeatingCallback<void(int)> cb) = 0;
  virtual void SetOnQueryChanged(base::RepeatingCallback<void(const std::string&)> cb) = 0;
  virtual void SetOnDismissed(base::RepeatingClosure cb) = 0;
};

}  // namespace veor
