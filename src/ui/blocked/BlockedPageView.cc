// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/blocked/BlockedPageView.h"

#include "base/i18n/rtl.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/background.h"

#include "ui/theme/IThemeProvider.h"

namespace veor {

namespace {

// VEOR identity colors
constexpr SkColor kBackground = SkColorSetRGB(0x05, 0x05, 0x05);
constexpr SkColor kCrimsonAccent = SkColorSetRGB(0x8B, 0x00, 0x00);
constexpr SkColor kTextPrimary = SkColorSetRGB(0xE8, 0xE8, 0xE8);
constexpr SkColor kTextSecondary = SkColorSetRGB(0x66, 0x66, 0x66);
constexpr SkColor kTextMuted = SkColorSetRGB(0x44, 0x44, 0x44);

std::string ThreatTypeToString(int type) {
  switch (type) {
    case 1: return "MALWARE";
    case 2: return "SOCIAL ENGINEERING";
    case 3: return "UNWANTED SOFTWARE";
    case 4: return "POTENTIALLY HARMFUL";
    default: return "UNKNOWN THREAT";
  }
}

}  // namespace

BlockedPageView::BlockedPageView(IThemeProvider* theme) : theme_(theme) {
  SetBackground(views::CreateSolidBackground(kBackground));
}

BlockedPageView::~BlockedPageView() = default;

void BlockedPageView::SetBlockedUrl(const std::string& url, int threat_type) {
  blocked_url_ = url;
  threat_type_ = threat_type;
  SchedulePaint();
}

void BlockedPageView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetLocalBounds();
  int center_x = bounds.width() / 2;
  int center_y = bounds.height() / 2;

  // Title: "BLOCKED"
  gfx::FontList title_font("Inter, sans-serif", gfx::Font::NORMAL, 48);
  std::u16string title_text = base::UTF8ToUTF16("BLOCKED");
  int title_w = canvas->GetStringWidth(title_text, title_font);
  canvas->DrawStringRectWithFlags(
      title_text,
      title_font,
      kCrimsonAccent,
      gfx::Rect(center_x - title_w / 2, center_y - 120, title_w, 60),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  // Divider line
  canvas->DrawLine(
      gfx::Point(center_x - 60, center_y - 50),
      gfx::Point(center_x + 60, center_y - 50),
      kCrimsonAccent,
      1.0f);

  // URL
  gfx::FontList url_font("Inter, sans-serif", gfx::Font::NORMAL, 14);
  std::u16string url_text = base::UTF8ToUTF16(blocked_url_);
  int url_w = canvas->GetStringWidth(url_text, url_font);
  canvas->DrawStringRectWithFlags(
      url_text,
      url_font,
      kTextSecondary,
      gfx::Rect(center_x - url_w / 2, center_y - 30, url_w, 24),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  // Threat type
  gfx::FontList threat_font("Inter, sans-serif", gfx::Font::NORMAL, 11);
  std::u16string threat_text = base::UTF8ToUTF16(
      "THREAT DETECTED: " + ThreatTypeToString(threat_type_));
  int threat_w = canvas->GetStringWidth(threat_text, threat_font);
  canvas->DrawStringRectWithFlags(
      threat_text,
      threat_font,
      kTextMuted,
      gfx::Rect(center_x - threat_w / 2, center_y + 10, threat_w, 20),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  // Subtitle
  gfx::FontList sub_font("Inter, sans-serif", gfx::Font::NORMAL, 12);
  std::u16string sub_text = base::UTF8ToUTF16(
      "VEOR has prevented navigation to this page.");
  int sub_w = canvas->GetStringWidth(sub_text, sub_font);
  canvas->DrawStringRectWithFlags(
      sub_text,
      sub_font,
      kTextMuted,
      gfx::Rect(center_x - sub_w / 2, center_y + 40, sub_w, 20),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  // Roman numeral marker
  gfx::FontList numeral_font("Inter, sans-serif", gfx::Font::NORMAL, 10);
  std::u16string numeral = base::UTF8ToUTF16("IV");
  int num_w = canvas->GetStringWidth(numeral, numeral_font);
  canvas->DrawStringRectWithFlags(
      numeral,
      numeral_font,
      kTextMuted,
      gfx::Rect(center_x - num_w / 2, center_y + 80, num_w, 16),
      gfx::Canvas::TEXT_ALIGN_CENTER);
}

void BlockedPageView::Layout() {
  // Centered composition — all positioning in OnPaint
}

}  // namespace veor