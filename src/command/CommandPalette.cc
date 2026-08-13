// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "command/CommandPalette.h"

#include <algorithm>

#include "ui/gfx/canvas.h"
#include "ui/painting/VeorPainter.h"

namespace veor {

CommandPalette::CommandPalette(IThemeProvider* theme) : theme_(theme) {}

CommandPalette::~CommandPalette() = default;

void CommandPalette::RegisterProvider(std::unique_ptr<ICommandProvider> provider) {
  providers_.push_back(std::move(provider));
}

void CommandPalette::Open() {
  is_open_ = true;
  SetVisible(true);
  SetQuery("");
  SchedulePaint();
}

void CommandPalette::Close() {
  is_open_ = false;
  SetVisible(false);
  current_results_.clear();
  current_query_.clear();
}

void CommandPalette::SetQuery(const std::string& query) {
  current_query_ = query;
  RebuildResults();
  SchedulePaint();
}

void CommandPalette::RebuildResults() {
  current_results_.clear();
  for (auto& provider : providers_) {
    auto items = provider->Query(current_query_);
    current_results_.insert(current_results_.end(),
                            std::make_move_iterator(items.begin()),
                            std::make_move_iterator(items.end()));
  }
  std::sort(current_results_.begin(), current_results_.end(),
            [](const CommandItem& a, const CommandItem& b) {
              return a.score > b.score;
            });
  selected_index_ = 0;
}

void CommandPalette::OnPaint(gfx::Canvas* canvas) {
  if (!is_open_) return;

  VeorPainter painter(theme_);
  SkRect bounds = SkRect::MakeWH(width(), height());
  painter.PaintSurface(canvas->sk_canvas(), bounds, ColorRole::kSurfacePrimary);
  painter.PaintBorder(canvas->sk_canvas(), bounds, ColorRole::kBorderPrimary, 1.0f);

  // Query input placeholder
  painter.PaintText(canvas->sk_canvas(), current_query_.empty() ? ">" : current_query_,
                    16, 28, ColorRole::kTextPrimary);

  // Results
  int y = 56;
  for (size_t i = 0; i < current_results_.size() && i < 10; ++i) {
    const auto& item = current_results_[i];
    ColorRole text_role = (static_cast<int>(i) == selected_index_)
                              ? ColorRole::kAccentCrimson
                              : ColorRole::kTextSecondary;
    painter.PaintText(canvas->sk_canvas(), item.title, 16, y, text_role);
    y += 24;
  }
}

void CommandPalette::Layout() {
  SetBounds((parent()->width() - 600) / 2, 80, 600, 400);
}

}  // namespace veor
