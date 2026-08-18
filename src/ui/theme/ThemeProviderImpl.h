// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <array>
#include <utility>

#include "ui/theme/IThemeProvider.h"

namespace veor {

class ThemeProviderImpl : public IThemeProvider {
 public:
  ThemeProviderImpl();

  SkColor GetColor(ColorRole role) const override;
  SecurityLineStyle GetSecurityLineStyle(bool secure, bool mixed) const override;

  std::string GetFontFamily() const override { return font_family_; }
  int GetFontSizePx() const override { return 13; }
  int GetFontSizeSmallPx() const override { return 10; }
  int GetFontSizeLargePx() const override { return 24; }
  int GetFontSizeMonumentalPx() const override { return 120; }

  int GetSpacingUnit() const override { return 4; }
  int GetBorderRadius() const override { return 0; }

  ThemeId GetCurrentTheme() const override { return current_theme_; }
  void SetTheme(ThemeId theme) override;

  int GetAnimationDurationMs() const override { return 250; }
  int GetAnimationDurationLongMs() const override { return 400; }
  int GetAnimationDurationStructuralMs() const override { return 600; }

 private:
  void BuildDarkPalette();

  ThemeId current_theme_ = ThemeId::kDark;
  std::string font_family_ = "Inter, system-ui, sans-serif";

  // Flat array of (role, color) pairs. Kept sorted for binary search.
  static constexpr size_t kPaletteSize = 35;
  std::array<std::pair<ColorRole, SkColor>, kPaletteSize> palette_;
};

}  // namespace veor
