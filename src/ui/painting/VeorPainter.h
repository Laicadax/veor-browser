// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ptr.h"
#include "ui/theme/IThemeProvider.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkPaint.h"

namespace veor {

class VeorPainter {
 public:
  explicit VeorPainter(IThemeProvider* theme);
  void PaintBackground(SkCanvas* canvas, const SkRect& bounds, ColorRole role);
  void PaintSurface(SkCanvas* canvas, const SkRect& bounds, ColorRole role);
  void PaintBorder(SkCanvas* canvas, const SkRect& bounds, ColorRole role, float thickness = 1.0f);
  void PaintFocusRing(SkCanvas* canvas, const SkRect& bounds, float stroke = 2.0f);
  void PaintText(SkCanvas* canvas, const std::string& text, float x, float y, ColorRole role);

 private:
  raw_ptr<IThemeProvider> theme_;
};

}  // namespace veor
