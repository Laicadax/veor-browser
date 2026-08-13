// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "third_party/skia/include/core/SkColor.h"

namespace veor {

enum class ColorRole {
  // ── V10 Deep-Void Palette ──
  kVoid,              // #020202 — absolute background, NTP
  kDepth,             // #080808 — workspace background
  kSurface,           // #0F0F0F — raised surfaces
  kEdge,              // #1A1A1A — seams between layers
  kWhisper,           // #252525 — hover elevation

  // Legacy aliases (mapped to V10 values)
  kBackgroundPrimary = kDepth,
  kBackgroundSecondary = kSurface,
  kBackgroundTertiary = kEdge,
  kSurfacePrimary = kSurface,
  kSurfaceSecondary = kWhisper,
  kBorderPrimary = kEdge,
  kBorderSecondary = kWhisper,

  // ── Text ──
  kTextPrimary,       // #E8E8E8
  kTextSecondary,     // #A0A0A0
  kTextTertiary,      // #606060
  kTextQuaternary,    // #404040 — almost invisible

  // ── Accent ──
  kAccentCrimson,      // #6B1520 — dried blood, not bright red
  kAccentCrimsonHover, // #8A1C2A
  kAccentCrimsonActive,// #4A0F18
  kCrimsonGlow,        // rgba(107,21,32,0.4) — focus rings, hover glow

  // ── Semantic (subdued, textural) ──
  kError,              // muted, not screaming
  kWarning,
  kSuccess,
  kFocusRing,          // uses kCrimsonGlow

  // ── Title Bar ──
  kTitleBarBackground,
  kTitleBarBorder,

  // ── Omnibox ──
  kOmniboxBackground,
  kOmniboxBorder,
  kOmniboxBorderSecure,
  kOmniboxBorderInsecure,
  kOmniboxBorderMixed,

  // ── Tab Strip ──
  kTabStripBackground,
  kTabHoverBackground,
  kTabActiveBorderBottom,
  kTabPinnedBorderBottom,
  kTabGroupColorDefault,

  // ── Buttons ──
  kButtonHoverBackground,
  kButtonActiveBackground,

  // ── Overlay / Toast ──
  kOverlayBackground,
  kOverlayBackdrop,
  kToastBorder,
  kToastProgressBar,
};

enum class SecurityLineStyle {
  kSolid,   // Secure
  kDashed,  // Insecure
  kNoise,   // Mixed
};

enum class ThemeId { kDark, kLight, kHighContrast };

class IThemeProvider {
 public:
  virtual ~IThemeProvider() = default;

  virtual SkColor GetColor(ColorRole role) const = 0;

  // Returns the security line style for the given state.
  // Not a color — a texture. The color is always kOmniboxBorder*.
  virtual SecurityLineStyle GetSecurityLineStyle(bool secure, bool mixed) const = 0;

  // Typography
  virtual std::string GetFontFamily() const = 0;
  virtual int GetFontSizePx() const = 0;          // Base: 13px
  virtual int GetFontSizeSmallPx() const = 0;     // 10px — labels, hints
  virtual int GetFontSizeLargePx() const = 0;     // 24px — command palette input
  virtual int GetFontSizeMonumentalPx() const = 0; // 96–180px — NTP logo

  // Spacing (mathematical, 4px grid)
  virtual int GetSpacingUnit() const = 0;         // 4px
  virtual int GetBorderRadius() const = 0;        // 0px — VEOR has no rounded corners

  virtual ThemeId GetCurrentTheme() const = 0;
  virtual void SetTheme(ThemeId theme) = 0;

  // Animation timing (slow, heavy, physical)
  virtual int GetAnimationDurationMs() const = 0;      // 250ms
  virtual int GetAnimationDurationLongMs() const = 0;  // 400ms
  virtual int GetAnimationDurationStructuralMs() const = 0; // 600ms
};

}  // namespace veor
