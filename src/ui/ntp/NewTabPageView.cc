// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/ntp/NewTabPageView.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(NewTabPageView)
END_METADATA

NewTabPageView::NewTabPageView(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetBackground(
      views::CreateSolidBackground(theme_->GetColor(ColorRole::kVoid)));
}

NewTabPageView::~NewTabPageView() = default;

void NewTabPageView::Show() {
  SetVisible(true);
  SchedulePaint();
}

void NewTabPageView::Hide() {
  SetVisible(false);
}

void NewTabPageView::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Absolute void — already handled by background, but ensure
  canvas->FillRect(GetLocalBounds(), theme_->GetColor(ColorRole::kVoid));

  int cx = width() / 2;
  int cy = height() / 2;

  // Logo: "VEOR" — monumental, tight tracking, not pure white
  gfx::FontList logo_font("Inter, 120px");
  std::u16string logo = u"VEOR";
  int logo_width = canvas->GetStringWidth(logo, logo_font);

  gfx::Rect logo_rect(cx - logo_width / 2, cy - 80,
                      logo_width, 140);

  canvas->DrawStringRect(
      logo, logo_rect, logo_font,
      gfx::Canvas::TEXT_ALIGN_CENTER,
      theme_->GetColor(ColorRole::kTextPrimary));

  // Tagline — whisper, uppercase, extreme letter-spacing
  gfx::FontList tag_font("Inter, 9px");
  std::u16string tagline = u"BEYOND THE ERA";
  int tag_width = canvas->GetStringWidth(tagline, tag_font);

  gfx::Rect tag_rect(cx - tag_width / 2, cy + 40,
                     tag_width, 20);

  canvas->DrawStringRect(
      tagline, tag_rect, tag_font,
      gfx::Canvas::TEXT_ALIGN_CENTER,
      theme_->GetColor(ColorRole::kTextQuaternary));
}

}  // namespace veor
