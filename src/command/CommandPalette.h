// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <vector>

#include "command/ICommandProvider.h"
#include "ui/theme/IThemeProvider.h"
#include "ui/views/view.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// CommandPalette — Modal command palette with providers
// ─────────────────────────────────────────────────────────────────────────────
// Opens with Ctrl+Shift+P. Searches across tabs, history, bookmarks, settings.
// ─────────────────────────────────────────────────────────────────────────────

class CommandPalette : public views::View {
 public:
  explicit CommandPalette(IThemeProvider* theme);
  ~CommandPalette() override;

  void RegisterProvider(std::unique_ptr<ICommandProvider> provider);
  void Open();
  void Close();
  bool IsOpen() const { return is_open_; }

  void SetQuery(const std::string& query);

 private:
  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;
  void RebuildResults();

  IThemeProvider* theme_;
  std::vector<std::unique_ptr<ICommandProvider>> providers_;
  std::vector<CommandItem> current_results_;
  std::string current_query_;
  bool is_open_ = false;
  int selected_index_ = 0;
};

}  // namespace veor
