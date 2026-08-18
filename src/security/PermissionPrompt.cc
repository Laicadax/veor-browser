// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "security/PermissionPrompt.h"
#include "url/gurl.h"

#include "ui/painting/VeorPainter.h"

namespace veor {

PermissionPrompt::PermissionPrompt(const GURL& origin,
                                   PermissionType type,
                                   IThemeProvider* theme,
                                   DecisionCallback callback)
    : origin_(origin),
      type_(type),
      theme_(theme),
      callback_(std::move(callback)) {}

PermissionPrompt::~PermissionPrompt() = default;

void PermissionPrompt::Init() {
  SetPreferredSize(gfx::Size(400, 160));
}

void PermissionPrompt::OnPaint(gfx::Canvas* canvas) {
  VeorPainter painter(theme_);
  SkRect bounds = SkRect::MakeWH(width(), height());
  painter.PaintSurface(canvas->sk_canvas(), bounds, ColorRole::kSurfacePrimary);
  painter.PaintBorder(canvas->sk_canvas(), bounds, ColorRole::kBorderPrimary, 1.0f);

  std::string title = "Permission Request";
  painter.PaintText(canvas->sk_canvas(), title, 20, 32, ColorRole::kTextPrimary);

  std::string msg = origin_.host() + " wants to access " + std::to_string(static_cast<int>(type_));
  painter.PaintText(canvas->sk_canvas(), msg, 20, 64, ColorRole::kTextSecondary);
}

void PermissionPrompt::Layout() {
  SetBounds((parent()->width() - 400) / 2,
            (parent()->height() - 160) / 2, 400, 160);
}

}  // namespace veor
