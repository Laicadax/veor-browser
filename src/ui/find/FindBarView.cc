// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/find/FindBarView.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/views/background.h"
#include "ui/views/border.h"

#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(FindBarView)
END_METADATA

namespace {

constexpr int kFindBarHeight = 36;
constexpr int kButtonWidth = 28;
constexpr int kSearchWidth = 240;
constexpr int kMatchWidth = 60;

}  // namespace

FindBarView::FindBarView(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurface)));
  SetBorder(views::CreateSolidBorder(
      1, theme_->GetColor(ColorRole::kEdge)));
}

FindBarView::~FindBarView() = default;

void FindBarView::Show() {
  SetVisible(true);
  SchedulePaint();
}

void FindBarView::Hide() {
  SetVisible(false);
  SchedulePaint();
}

bool FindBarView::IsVisible() const {
  return views::View::GetVisible();
}

void FindBarView::SetMatchCount(int active_match, int total_matches) {
  if (total_matches > 0) {
    match_text_ = base::UTF8ToUTF16(
        std::to_string(active_match) + "/" + std::to_string(total_matches));
  } else {
    match_text_ = u"0/0";
  }
  SchedulePaint();
}

void FindBarView::SetOnFindRequested(
    base::RepeatingCallback<void(const std::string& text,
                                 bool forward,
                                 bool match_case)> cb) {
  on_find_ = std::move(cb);
}

void FindBarView::SetOnCloseRequested(base::RepeatingClosure cb) {
  on_close_ = std::move(cb);
}

void FindBarView::FocusSearchBox() {
  // Focus is handled by the shell focusing this view
  RequestFocus();
}

gfx::Size FindBarView::CalculatePreferredSize() const {
  return gfx::Size(kSearchWidth + kMatchWidth + kButtonWidth * 3 + 24,
                   kFindBarHeight);
}

void FindBarView::Layout() {
  // Simple layout: search box on left, match count, prev/next/close on right
  int x = 8;
  if (search_box_)
    search_box_->SetBounds(x, 4, kSearchWidth, kFindBarHeight - 8);
}

void FindBarView::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  gfx::Rect bounds = GetLocalBounds();
  canvas->FillRect(bounds, theme_->GetColor(ColorRole::kSurface));

  // Top edge — thin crimson line
  SkPaint top_paint;
  top_paint.setColor(theme_->GetColor(ColorRole::kAccentCrimson));
  top_paint.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(0, 0, bounds.width(), 0, top_paint);

  gfx::FontList font("Inter, 12px");
  SkColor text_color = theme_->GetColor(ColorRole::kTextPrimary);
  SkColor muted = theme_->GetColor(ColorRole::kTextQuaternary);

  int x = 8;
  int y = (bounds.height() - 16) / 2;

  // Search text
  if (!search_text_.empty()) {
    canvas->DrawStringRect(search_text_,
                           gfx::Rect(x, y, kSearchWidth, 16),
                           font, gfx::Canvas::TEXT_ALIGN_LEFT, text_color);
  } else {
    canvas->DrawStringRect(u"Find in page…",
                           gfx::Rect(x, y, kSearchWidth, 16),
                           font, gfx::Canvas::TEXT_ALIGN_LEFT, muted);
  }
  x += kSearchWidth + 12;

  // Match count
  if (!match_text_.empty()) {
    canvas->DrawStringRect(match_text_,
                           gfx::Rect(x, y, kMatchWidth, 16),
                           font, gfx::Canvas::TEXT_ALIGN_LEFT, muted);
  }
  x += kMatchWidth + 12;

  // Prev button (←)
  gfx::Rect prev_rect(x, 4, kButtonWidth, kFindBarHeight - 8);
  SkPaint btn_bg;
  btn_bg.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
  canvas->DrawRect(prev_rect, btn_bg);
  canvas->DrawStringRect(u"\u2191", prev_rect, gfx::FontList("Inter, 14px"),
                         gfx::Canvas::TEXT_ALIGN_CENTER, text_color);
  x += kButtonWidth + 2;

  // Next button (↓)
  gfx::Rect next_rect(x, 4, kButtonWidth, kFindBarHeight - 8);
  canvas->DrawRect(next_rect, btn_bg);
  canvas->DrawStringRect(u"\u2193", next_rect, gfx::FontList("Inter, 14px"),
                         gfx::Canvas::TEXT_ALIGN_CENTER, text_color);
  x += kButtonWidth + 2;

  // Close button (×)
  gfx::Rect close_rect(x, 4, kButtonWidth, kFindBarHeight - 8);
  canvas->DrawRect(close_rect, btn_bg);
  canvas->DrawStringRect(u"\u00D7", close_rect, gfx::FontList("Inter, 14px"),
                         gfx::Canvas::TEXT_ALIGN_CENTER,
                         theme_->GetColor(ColorRole::kAccentCrimson));
}

bool FindBarView::OnMousePressed(const ui::MouseEvent& event) {
  int x = event.x();
  int y = event.y();

  int btn_x = 8 + kSearchWidth + 12 + kMatchWidth + 12;

  // Prev button
  if (x >= btn_x && x < btn_x + kButtonWidth) {
    if (on_find_)
      on_find_.Run(base::UTF16ToUTF8(search_text_), false, match_case_);
    return true;
  }
  btn_x += kButtonWidth + 2;

  // Next button
  if (x >= btn_x && x < btn_x + kButtonWidth) {
    if (on_find_)
      on_find_.Run(base::UTF16ToUTF8(search_text_), true, match_case_);
    return true;
  }
  btn_x += kButtonWidth + 2;

  // Close button
  if (x >= btn_x && x < btn_x + kButtonWidth) {
    if (on_close_)
      on_close_.Run();
    return true;
  }

  // Click in search area — focus for text input
  if (x < 8 + kSearchWidth) {
    // TODO: Implement text input handling
    return true;
  }

  return views::View::OnMousePressed(event);
}

}  // namespace veor
