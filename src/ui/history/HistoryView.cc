// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/history/HistoryView.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/painting/VeorPainter.h"
#include "ui/theme/IThemeProvider.h"
#include "history/HistoryStoreImpl.h"

namespace veor {

BEGIN_METADATA(HistoryView)
END_METADATA

HistoryView::HistoryView(IThemeProvider* theme, IHistoryStore* history_store)
    : theme_(theme), history_store_(history_store) {
  DCHECK(theme_);
  LoadEntries();
}

HistoryView::~HistoryView() = default;

void HistoryView::OnPaint(gfx::Canvas* canvas) {
  DrawHeader(canvas);
  DrawEntries(canvas);
}

void HistoryView::Layout() {
  views::View::Layout();
}

void HistoryView::Reload() {
  LoadEntries();
  SchedulePaint();
}

void HistoryView::SetOnEntrySelected(base::RepeatingCallback<void(const GURL&)> cb) {
  on_entry_selected_ = std::move(cb);
}

bool HistoryView::OnMousePressed(const ui::MouseEvent& event) {
  int y = event.y() - header_height_ + scroll_offset_;
  int index = y / entry_height_;
  if (index >= 0 && static_cast<size_t>(index) < entries_.size()) {
    if (on_entry_selected_)
      on_entry_selected_.Run(entries_[index].url);
  }
  return true;
}

void HistoryView::DrawHeader(gfx::Canvas* canvas) {
  gfx::Rect header(0, 0, width(), header_height_);
  canvas->FillRect(header, theme_->GetColor(ColorRole::kSurface));

  VeorPainter painter(theme_);
  painter.PaintText(canvas, "HISTORY", 16, 28, ColorRole::kForeground);

  // Divider
  canvas->FillRect(
      gfx::Rect(0, header_height_ - 1, width(), 1),
      theme_->GetColor(ColorRole::kEdge));
}

void HistoryView::DrawEntries(gfx::Canvas* canvas) {
  int y = header_height_;
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (y + entry_height_ < 0 || y > height()) {
      y += entry_height_;
      continue;
    }

    gfx::Rect item_rect(0, y, width(), entry_height_);

    // Hover / selection — none for MVP, uniform surface
    canvas->FillRect(item_rect, theme_->GetColor(ColorRole::kSurface));

    // Title
    VeorPainter painter(theme_);
    std::string title = entries_[i].title.empty()
                            ? entries_[i].url.spec()
                            : entries_[i].title;
    painter.PaintText(canvas, title, 16, y + 16, ColorRole::kForeground);

    // URL
    painter.PaintText(canvas, entries_[i].url.spec(), 16, y + 28,
                      ColorRole::kForegroundMuted);

    // Divider
    canvas->FillRect(gfx::Rect(16, y + entry_height_ - 1,
                               width() - 32, 1),
                     theme_->GetColor(ColorRole::kEdge));

    y += entry_height_;
  }
}

void HistoryView::LoadEntries() {
  if (!history_store_)
    return;
  auto result = history_store_->GetRecent(100);
  if (result.IsOk()) {
    entries_ = std::move(result).Unwrap();
  }
}

}  // namespace veor
