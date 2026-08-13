// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/omnibox/OmniboxView.h"

#include "ui/gfx/canvas.h"
#include "ui/theme/IThemeProvider.h"
#include "third_party/skia/include/effects/SkDashPathEffect.h"

namespace veor {

BEGIN_METADATA(OmniboxView)
END_METADATA

OmniboxView::OmniboxView(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetBorder(nullptr);
  SetBackgroundColor(theme_->GetColor(ColorRole::kOmniboxBackground));
  SetTextColor(theme_->GetColor(ColorRole::kTextSecondary));
  SetFontList(gfx::FontList("Inter, 13px"));
}

OmniboxView::~OmniboxView() = default;

void OmniboxView::SetText(const std::string& text) {
  views::Textfield::SetText(base::UTF8ToUTF16(text));
}

std::string OmniboxView::GetText() const {
  return base::UTF16ToUTF8(views::Textfield::GetText());
}

void OmniboxView::SetSecure(bool secure, bool mixed) {
  secure_ = secure;
  mixed_ = mixed;
  SchedulePaint();
}

void OmniboxView::SetLoading(bool loading) {
  loading_ = loading;
  SchedulePaint();
}

void OmniboxView::Focus() {
  views::Textfield::RequestFocus();
}

void OmniboxView::Blur() {
  if (GetFocusManager())
    GetFocusManager()->SetFocusedView(nullptr);
}

void OmniboxView::SetOnCommit(base::RepeatingCallback<void(const std::string&)> cb) {
  on_commit_ = std::move(cb);
}

void OmniboxView::SetOnFocusChanged(base::RepeatingCallback<void(bool)> cb) {
  on_focus_changed_ = std::move(cb);
}

void OmniboxView::OnPaintBorder(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  gfx::Rect bounds = GetLocalBounds();
  SkPaint border_paint;

  if (loading_) {
    border_paint.setColor(theme_->GetColor(ColorRole::kCrimsonGlow));
    border_paint.setStrokeWidth(2);
  } else if (HasFocus()) {
    border_paint.setColor(theme_->GetColor(ColorRole::kAccentCrimson));
    border_paint.setStrokeWidth(2);
  } else {
    SecurityLineStyle style = theme_->GetSecurityLineStyle(secure_, mixed_);
    border_paint.setColor(theme_->GetColor(ColorRole::kOmniboxBorder));
    border_paint.setStrokeWidth(1);

    if (style == SecurityLineStyle::kDashed) {
      SkScalar intervals[] = {4, 4};
      border_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    } else if (style == SecurityLineStyle::kNoise) {
      SkScalar intervals[] = {1, 2};
      border_paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    }
  }

  canvas->sk_canvas()->drawLine(0, bounds.bottom() - 0.5f,
                                bounds.width(), bounds.bottom() - 0.5f,
                                border_paint);
}

void OmniboxView::OnFocus() {
  views::Textfield::OnFocus();
  SetTextColor(theme_->GetColor(ColorRole::kTextPrimary));
  if (on_focus_changed_)
    on_focus_changed_.Run(true);
}

void OmniboxView::OnBlur() {
  views::Textfield::OnBlur();
  SetTextColor(theme_->GetColor(ColorRole::kTextSecondary));
  if (on_focus_changed_)
    on_focus_changed_.Run(false);
}

bool OmniboxView::HandleKeyEvent(views::Textfield* sender,
                                 const ui::KeyEvent& key_event) {
  if (key_event.type() == ui::ET_KEY_PRESSED &&
      key_event.key_code() == ui::VKEY_RETURN) {
    if (on_commit_)
      on_commit_.Run(GetText());
    return true;
  }
  return views::Textfield::HandleKeyEvent(sender, key_event);
}

}  // namespace veor
