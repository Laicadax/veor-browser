// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;
class BookmarkDatabase;
struct BookmarkEntry;

// ─────────────────────────────────────────────────────────────────────────────
// BookmarksView
// ─────────────────────────────────────────────────────────────────────────────
// Displays bookmarks as a scrollable tree.
// Opens via Command Palette "> Bookmarks" or Ctrl+Shift+B.
// ─────────────────────────────────────────────────────────────────────────────

class BookmarksView : public views::View {
  METADATA_HEADER(BookmarksView, views::View)

 public:
  explicit BookmarksView(IThemeProvider* theme,
                         IBookmarkStore* bookmark_store);
  ~BookmarksView() override;

  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;

  // Refresh from database
  void Reload();

  // Callback when user selects a bookmark
  void SetOnBookmarkSelected(base::RepeatingCallback<void(const GURL&)> cb);

  // Mouse
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  void DrawHeader(gfx::Canvas* canvas);
  void DrawEntries(gfx::Canvas* canvas);
  void LoadEntries();

  IThemeProvider* theme_ = nullptr;
  IBookmarkStore* bookmark_store_ = nullptr;

  std::vector<BookmarkNode> entries_;
  base::RepeatingCallback<void(const GURL&)> on_bookmark_selected_;

  int scroll_offset_ = 0;
  int entry_height_ = 36;
  int header_height_ = 48;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
