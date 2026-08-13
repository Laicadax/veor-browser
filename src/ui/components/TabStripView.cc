// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/components/TabStripView.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(TabStripView)
END_METADATA

TabStripView::TabStripView(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetPreferredSize(gfx::Size(0, tab_height_));
}

TabStripView::~TabStripView() = default;

void TabStripView::SetTabs(const std::vector<TabVisual>& tabs) {
  tabs_ = tabs;
  SchedulePaint();
}

void TabStripView::SetOnTabSelected(base::RepeatingCallback<void(size_t)> cb) {
  on_tab_selected_ = std::move(cb);
}

void TabStripView::SetOnTabClosed(base::RepeatingCallback<void(size_t)> cb) {
  on_tab_closed_ = std::move(cb);
}

void TabStripView::SetOnTabContextMenu(
    base::RepeatingCallback<void(size_t, gfx::Point)> cb) {
  on_tab_context_menu_ = std::move(cb);
}

bool TabStripView::OnMousePressed(const ui::MouseEvent& event) {
  if (tabs_.empty())
    return views::View::OnMousePressed(event);

  int x = event.x();
  int total_width = width();
  int available = total_width - 1;
  int tab_width = tabs_.size() > 0
                      ? std::clamp(static_cast<int>(available / static_cast<int>(tabs_.size())),
                                   tab_min_width_, tab_max_width_)
                      : tab_min_width_;

  int tab_x = 1;
  for (size_t i = 0; i < tabs_.size(); ++i) {
    int w = tabs_[i].pinned ? 32 : tab_width;
    if (tab_x + w > total_width) w = total_width - tab_x;

    if (x >= tab_x && x < tab_x + w) {
      if (event.IsRightMouseButton() && on_tab_context_menu_) {
        on_tab_context_menu_.Run(i, event.location());
        return true;
      }
      // Check close button click first
      if (!tabs_[i].pinned && x >= tab_x + w - 16 && on_tab_closed_) {
        on_tab_closed_.Run(i);
        return true;
      }
      if (event.IsMiddleMouseButton() && on_tab_closed_) {
        on_tab_closed_.Run(i);
        return true;
      }
      if (on_tab_selected_) {
        on_tab_selected_.Run(i);
        return true;
      }
    }

    tab_x += w;
    if (tab_x >= total_width) break;
  }

  return views::View::OnMousePressed(event);
}

void TabStripView::OnMouseMoved(const ui::MouseEvent& event) {
  if (tabs_.empty()) {
    if (hovered_tab_ != static_cast<size_t>(-1)) {
      hovered_tab_ = static_cast<size_t>(-1);
      hovered_close_ = false;
      SchedulePaint();
    }
    return;
  }

  int x = event.x();
  int total_width = width();
  int available = total_width - 1;
  int tab_width = tabs_.size() > 0
                      ? std::clamp(static_cast<int>(available / static_cast<int>(tabs_.size())),
                                   tab_min_width_, tab_max_width_)
                      : tab_min_width_;

  int tab_x = 1;
  size_t new_hovered = static_cast<size_t>(-1);
  bool new_hovered_close = false;

  for (size_t i = 0; i < tabs_.size(); ++i) {
    int w = tabs_[i].pinned ? 32 : tab_width;
    if (tab_x + w > total_width) w = total_width - tab_x;

    if (x >= tab_x && x < tab_x + w) {
      new_hovered = i;
      // Check if hovering close button (right 16px of non-pinned tab)
      if (!tabs_[i].pinned && x >= tab_x + w - 16) {
        new_hovered_close = true;
      }
      break;
    }

    tab_x += w;
    if (tab_x >= total_width) break;
  }

  if (new_hovered != hovered_tab_ || new_hovered_close != hovered_close_) {
    hovered_tab_ = new_hovered;
    hovered_close_ = new_hovered_close;
    SchedulePaint();
  }
}

void TabStripView::OnMouseExited(const ui::MouseEvent& event) {
  if (hovered_tab_ != static_cast<size_t>(-1)) {
    hovered_tab_ = static_cast<size_t>(-1);
    hovered_close_ = false;
    SchedulePaint();
  }
}

void TabStripView::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  canvas->FillRect(GetLocalBounds(),
                   theme_->GetColor(ColorRole::kTabStripBackground));

  if (tabs_.empty()) return;

  int total_width = width();
  int available = total_width - 1;
  int tab_width = tabs_.size() > 0
                      ? std::clamp(static_cast<int>(available / static_cast<int>(tabs_.size())),
                                   tab_min_width_, tab_max_width_)
                      : tab_min_width_;

  int x = 1;
  for (size_t i = 0; i < tabs_.size(); ++i) {
    const auto& tab = tabs_[i];
    int w = tab.pinned ? 32 : tab_width;
    if (x + w > total_width) w = total_width - x;

    gfx::Rect tab_rect(x, 0, w, tab_height_);

    // Hover background
    if (i == hovered_tab_ && !tab.active) {
      SkPaint hover_paint;
      hover_paint.setColor(theme_->GetColor(ColorRole::kTabHoverBackground));
      canvas->DrawRect(tab_rect, hover_paint);
    }

    if (tab.pinned) {
      SkPaint line_paint;
      line_paint.setColor(tab.domain_color != SK_ColorTRANSPARENT
                              ? tab.domain_color
                              : theme_->GetColor(ColorRole::kTabPinnedBorderBottom));
      line_paint.setStrokeWidth(2);
      canvas->sk_canvas()->drawLine(x + 2, 6, x + 2, tab_height_ - 6,
                                    line_paint);
    } else {
      gfx::FontList font("Inter, 11px");
      std::u16string title = base::UTF8ToUTF16(
          tab.title.empty() ? "..." : tab.title);

      SkColor text_color = tab.active
                               ? theme_->GetColor(ColorRole::kTextPrimary)
                               : theme_->GetColor(ColorRole::kTextQuaternary);

      // Close button area reduces text width
      int text_right_inset = (i == hovered_tab_) ? 20 : 8;
      gfx::Rect text_rect = tab_rect;
      text_rect.Inset(8, 0, text_right_inset, 0);
      canvas->DrawStringRect(title, text_rect, font,
                             gfx::Canvas::TEXT_ALIGN_LEFT, text_color);

      // Close button on hover
      if (i == hovered_tab_) {
        gfx::Rect close_rect(x + w - 18, (tab_height_ - 12) / 2, 12, 12);
        SkColor close_color = hovered_close_
                                  ? theme_->GetColor(ColorRole::kAccentCrimson)
                                  : theme_->GetColor(ColorRole::kTextQuaternary);
        canvas->DrawStringRect(u"\u00D7", close_rect, gfx::FontList("Inter, 12px"),
                               gfx::Canvas::TEXT_ALIGN_CENTER, close_color);
      }
    }

    if (tab.active) {
      SkPaint active_paint;
      active_paint.setColor(theme_->GetColor(ColorRole::kTabActiveBorderBottom));
      active_paint.setStrokeWidth(2);
      canvas->sk_canvas()->drawLine(x, tab_height_ - 1, x + w, tab_height_ - 1,
                                    active_paint);
    }

    if (tab.audio_playing && !tab.active) {
      SkPaint audio_paint;
      audio_paint.setColor(theme_->GetColor(ColorRole::kCrimsonGlow));
      audio_paint.setStrokeWidth(1);
      canvas->sk_canvas()->drawLine(x + 4, tab_height_ - 2, x + w - 4,
                                    tab_height_ - 2, audio_paint);
    }

    x += w;
    if (x >= total_width) break;
  }
}

gfx::Size TabStripView::CalculatePreferredSize() const {
  return gfx::Size(views::View::kUsePreferredSize, tab_height_);
}

}  // namespace veor
