// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/theme/ThemeProviderImpl.h"

#include <algorithm>

namespace veor {

namespace {

constexpr SkColor RGB(uint8_t r, uint8_t g, uint8_t b) {
  return SkColorSetRGB(r, g, b);
}

}  // namespace

ThemeProviderImpl::ThemeProviderImpl() {
  BuildDarkPalette();
}

void ThemeProviderImpl::BuildDarkPalette() {
  palette_ = {{
    {ColorRole::kVoid,                   RGB(0x02, 0x02, 0x02)},
    {ColorRole::kDepth,                  RGB(0x08, 0x08, 0x08)},
    {ColorRole::kSurface,                RGB(0x0F, 0x0F, 0x0F)},
    {ColorRole::kEdge,                   RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kWhisper,                RGB(0x25, 0x25, 0x25)},
    {ColorRole::kTextPrimary,            RGB(0xE8, 0xE8, 0xE8)},
    {ColorRole::kTextSecondary,          RGB(0xA0, 0xA0, 0xA0)},
    {ColorRole::kTextTertiary,           RGB(0x60, 0x60, 0x60)},
    {ColorRole::kTextQuaternary,         RGB(0x40, 0x40, 0x40)},
    {ColorRole::kAccentCrimson,          RGB(0x6B, 0x15, 0x20)},
    {ColorRole::kAccentCrimsonHover,     RGB(0x8A, 0x1C, 0x2A)},
    {ColorRole::kAccentCrimsonActive,    RGB(0x4A, 0x0F, 0x18)},
    {ColorRole::kCrimsonGlow,            SkColorSetARGB(0x66, 0x6B, 0x15, 0x20)},
    {ColorRole::kError,                  RGB(0x80, 0x20, 0x20)},
    {ColorRole::kWarning,                RGB(0x80, 0x60, 0x20)},
    {ColorRole::kSuccess,                RGB(0x20, 0x60, 0x40)},
    {ColorRole::kFocusRing,              RGB(0x6B, 0x15, 0x20)},
    {ColorRole::kTitleBarBackground,     RGB(0x08, 0x08, 0x08)},
    {ColorRole::kTitleBarBorder,         RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kOmniboxBackground,      RGB(0x02, 0x02, 0x02)},
    {ColorRole::kOmniboxBorder,          RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kOmniboxBorderSecure,    RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kOmniboxBorderInsecure,  RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kOmniboxBorderMixed,     RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kTabStripBackground,     RGB(0x08, 0x08, 0x08)},
    {ColorRole::kTabHoverBackground,     RGB(0x14, 0x14, 0x14)},
    {ColorRole::kTabActiveBorderBottom,  RGB(0x6B, 0x15, 0x20)},
    {ColorRole::kTabPinnedBorderBottom,  RGB(0x40, 0x40, 0x40)},
    {ColorRole::kTabGroupColorDefault,   RGB(0x6B, 0x15, 0x20)},
    {ColorRole::kButtonHoverBackground,  RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kButtonActiveBackground, RGB(0x25, 0x25, 0x25)},
    {ColorRole::kOverlayBackground,      RGB(0x08, 0x08, 0x08)},
    {ColorRole::kOverlayBackdrop,        RGB(0x02, 0x02, 0x02)},
    {ColorRole::kToastBorder,            RGB(0x1A, 0x1A, 0x1A)},
    {ColorRole::kToastProgressBar,       RGB(0x6B, 0x15, 0x20)},
  }};

  std::sort(palette_.begin(), palette_.end(),
            [](const auto& a, const auto& b) {
              return static_cast<int>(a.first) < static_cast<int>(b.first);
            });
}

SkColor ThemeProviderImpl::GetColor(ColorRole role) const {
  auto it = std::lower_bound(
      palette_.begin(), palette_.end(), role,
      [](const auto& pair, ColorRole r) {
        return static_cast<int>(pair.first) < static_cast<int>(r);
      });
  if (it != palette_.end() && it->first == role)
    return it->second;
  return RGB(0xFF, 0x00, 0xFF);
}

SecurityLineStyle ThemeProviderImpl::GetSecurityLineStyle(bool secure,
                                                         bool mixed) const {
  if (mixed) return SecurityLineStyle::kNoise;
  if (!secure) return SecurityLineStyle::kDashed;
  return SecurityLineStyle::kSolid;
}

void ThemeProviderImpl::SetTheme(ThemeId theme) {
  current_theme_ = theme;
  if (theme == ThemeId::kDark) BuildDarkPalette();
}

}  // namespace veor
