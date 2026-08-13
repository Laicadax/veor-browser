// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/bookmarks/BookmarksView.h"

#include "ui/gfx/canvas.h"
#include "ui/painting/VeorPainter.h"
#include "ui/theme/IThemeProvider.h"
#include "bookmarks/BookmarkStoreImpl.h"

namespace veor {

BEGIN_METADATA(BookmarksView)
END_METADATA

BookmarksView::BookmarksView(IThemeProvider* theme,
                               BookmarkDatabase* bookmark_db)
    : theme_(theme), bookmark_db_(bookmark_db) {
  DCHECK(theme_);
  LoadEntries();
}

BookmarksView::~BookmarksView() = default;

void BookmarksView::OnPaint(gfx::Canvas* canvas) {
  DrawHeader(canvas);
  DrawEntries(canvas);
}

void BookmarksView::Layout() {
  views::View::Layout();
}

void BookmarksView::Reload() {
  LoadEntries();
  SchedulePaint();
}

void BookmarksView::SetOnBookmarkSelected(
    base::RepeatingCallback<void(const GURL&)> cb) {
  on_bookmark_selected_ = std::move(cb);
}

bool BookmarksView::OnMousePressed(const ui::MouseEvent& event) {
  int y = event.y() - header_height_ + scroll_offset_;
  int index = y / entry_height_;
  if (index >= 0 && static_cast<size_t>(index) < entries_.size()) {
    if (!entries_[index].is_folder && on_bookmark_selected_)
      on_bookmark_selected_.Run(entries_[index].url);
  }
  return true;
}

void BookmarksView::DrawHeader(gfx::Canvas* canvas) {
  gfx::Rect header(0, 0, width(), header_height_);
  canvas->FillRect(header, theme_->GetColor(ColorRole::kSurface));

  VeorPainter painter(theme_);
  painter.PaintText(canvas, "BOOKMARKS", 16, 28, ColorRole::kForeground);

  canvas->FillRect(
      gfx::Rect(0, header_height_ - 1, width(), 1),
      theme_->GetColor(ColorRole::kEdge));
}

void BookmarksView::DrawEntries(gfx::Canvas* canvas) {
  int y = header_height_;
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (y + entry_height_ < 0 || y > height()) {
      y += entry_height_;
      continue;
    }

    gfx::Rect item_rect(0, y, width(), entry_height_);
    canvas->FillRect(item_rect, theme_->GetColor(ColorRole::kSurface));

    VeorPainter painter(theme_);
    int indent = entries_[i].parent_id == BookmarkNodeId() ? 16 : 32;

    // Folder icon or bookmark indicator
    std::string prefix = entries_[i].type == BookmarkNodeType::kFolder ? "> " : "  ";
    painter.PaintText(canvas, prefix + entries_[i].title,
                      indent, y + 20, ColorRole::kForeground);

    if (!entries_[i].is_folder) {
      painter.PaintText(canvas, entries_[i].url.spec(),
                        indent + 20, y + 30,
                        ColorRole::kForegroundMuted);
    }

    canvas->FillRect(
        gfx::Rect(16, y + entry_height_ - 1, width() - 32, 1),
        theme_->GetColor(ColorRole::kEdge));

    y += entry_height_;
  }
}

void BookmarksView::LoadEntries() {
  if (!bookmark_store_)
    return;
  auto result = bookmark_store_->GetChildren(bookmark_store_->GetRootId());
  if (result.IsOk()) {
    entries_ = std::move(result).Unwrap();
  }
}

}  // namespace veor
