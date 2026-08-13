// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/menus/ContextMenus.h"

#include "ui/painting/VeorPainter.h"

namespace veor {

ContextMenu::ContextMenu(IThemeProvider* theme) : theme_(theme) {}

ContextMenu::~ContextMenu() = default;

void ContextMenu::AddItem(const MenuItem& item) {
  items_.push_back(item);
}

void ContextMenu::ShowAt(const gfx::Point& screen_point) {
  int height = static_cast<int>(items_.size()) * item_height_;
  SetBounds(screen_point.x(), screen_point.y(), menu_width_, height);
  SetVisible(true);
  SchedulePaint();
}

void ContextMenu::Dismiss() {
  SetVisible(false);
  items_.clear();
}

void ContextMenu::OnPaint(gfx::Canvas* canvas) {
  VeorPainter painter(theme_);
  SkRect bounds = SkRect::MakeWH(width(), height());
  painter.PaintSurface(canvas->sk_canvas(), bounds, ColorRole::kSurfacePrimary);
  painter.PaintBorder(canvas->sk_canvas(), bounds, ColorRole::kBorderPrimary, 1.0f);

  int y = 0;
  for (const auto& item : items_) {
    if (item.separator) {
      SkPaint paint;
      paint.setColor(theme_->GetColor(ColorRole::kBorderPrimary));
      paint.setStrokeWidth(1);
      canvas->sk_canvas()->drawLine(8, y + item_height_ / 2,
                                    width() - 8, y + item_height_ / 2, paint);
    } else {
      ColorRole text_role = item.enabled ? ColorRole::kTextSecondary
                                          : ColorRole::kTextTertiary;
      painter.PaintText(canvas->sk_canvas(), item.label, 12,
                        y + item_height_ * 0.7f, text_role);
    }
    y += item_height_;
  }
}

void ContextMenu::Layout() {}

}  // namespace veor
