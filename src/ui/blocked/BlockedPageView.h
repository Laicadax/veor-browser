// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include "base/memory/raw_ptr.h"

#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// BlockedPageView
// ─────────────────────────────────────────────────────────────────────────────
// Displayed when Safe Browsing blocks a malicious URL.
// VEOR identity: deep black surface, crimson accent, monumental typography.
// No decorative elements. Silence and precision.
// ─────────────────────────────────────────────────────────────────────────────

class BlockedPageView : public views::View {
 public:
  explicit BlockedPageView(IThemeProvider* theme);
  ~BlockedPageView() override;

  // Set the blocked URL and threat type for display
  void SetBlockedUrl(const std::string& url, int threat_type);

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;

 private:
  void DrawBackground(gfx::Canvas* canvas);
  void DrawTitle(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawUrl(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawThreatInfo(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawActionButton(gfx::Canvas* canvas, const gfx::Rect& bounds);

  raw_ptr<IThemeProvider> theme_= nullptr;
  std::string blocked_url_;
  int threat_type_ = 0;

  // Layout geometry — computed in Layout()
  gfx::Rect title_bounds_;
  gfx::Rect url_bounds_;
  gfx::Rect threat_bounds_;
  gfx::Rect button_bounds_;
};

}  // namespace veor
