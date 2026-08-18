// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/painting/VeorPainter.h"

#include "third_party/skia/include/core/SkFont.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace veor {

VeorPainter::VeorPainter(IThemeProvider* theme) : theme_(theme) {}

void VeorPainter::PaintBackground(SkCanvas* canvas, const SkRect& bounds, ColorRole role) {
  SkPaint paint;
  paint.setColor(theme_->GetColor(role));
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(bounds, paint);
}

void VeorPainter::PaintSurface(SkCanvas* canvas, const SkRect& bounds, ColorRole role) {
  SkPaint paint;
  paint.setColor(theme_->GetColor(role));
  paint.setStyle(SkPaint::kFill_Style);
  paint.setAntiAlias(true);
  canvas->drawRect(bounds, paint);
}

void VeorPainter::PaintBorder(SkCanvas* canvas, const SkRect& bounds, ColorRole role, float thickness) {
  SkPaint paint;
  paint.setColor(theme_->GetColor(role));
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(thickness);
  paint.setAntiAlias(true);
  canvas->drawRect(bounds, paint);
}

void VeorPainter::PaintFocusRing(SkCanvas* canvas, const SkRect& bounds, float stroke) {
  SkPaint paint;
  paint.setColor(theme_->GetColor(ColorRole::kFocusRing));
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(stroke);
  paint.setAntiAlias(true);
  SkRect outset = bounds.makeOutset(stroke * 0.5f, stroke * 0.5f);
  canvas->drawRect(outset, paint);
}

void VeorPainter::PaintText(SkCanvas* canvas, const std::string& text, float x, float y, ColorRole role) {
  SkPaint paint;
  paint.setColor(theme_->GetColor(role));
  paint.setAntiAlias(true);
  SkFont font;
  font.setSize(13.0f);
  canvas->drawString(text.c_str(), x, y, font, paint);
}

}  // namespace veor
