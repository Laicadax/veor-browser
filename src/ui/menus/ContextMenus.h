// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ui/theme/IThemeProvider.h"
#include "ui/views/view.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// MenuItem
// ─────────────────────────────────────────────────────────────────────────────

struct MenuItem {
  std::string label;
  std::string shortcut;
  bool enabled = true;
  bool separator = false;
  std::function<void()> action;
};

// ─────────────────────────────────────────────────────────────────────────────
// ContextMenu — Right-click menu for tabs, content, bookmarks
// ─────────────────────────────────────────────────────────────────────────────

class ContextMenu : public views::View {
 public:
  explicit ContextMenu(IThemeProvider* theme);
  ~ContextMenu() override;

  void AddItem(const MenuItem& item);
  void ShowAt(const gfx::Point& screen_point);
  void Dismiss();

 private:
  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;

  IThemeProvider* theme_;
  std::vector<MenuItem> items_;
  int item_height_ = 26;
  int menu_width_ = 200;
};

}  // namespace veor
