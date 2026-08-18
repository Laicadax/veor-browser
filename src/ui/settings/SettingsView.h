// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <memory>
#include "base/memory/raw_ptr.h"
#include <string>
#include <vector>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class ISettingsProvider;
class IThemeProvider;
struct SettingDef;

// ─────────────────────────────────────────────────────────────────────────────
// SettingsView
// ─────────────────────────────────────────────────────────────────────────────
// Two-pane settings panel. Left: category list. Right: controls.
//
// No rounded corners. No gradients. Just lines, text, and switches.
// ─────────────────────────────────────────────────────────────────────────────

class SettingsView : public views::View {
  METADATA_HEADER(SettingsView, views::View)

 public:
  SettingsView(IThemeProvider* theme, ISettingsProvider* provider);
  ~SettingsView() override;

  void Show();
  void Hide();
  bool IsVisible() const;

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;

 private:
  void DrawCategoryList(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawSettingsPanel(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void DrawSettingRow(gfx::Canvas* canvas,
                      const SettingDef* def,
                      const gfx::Rect& row_bounds,
                      int row_index);
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  raw_ptr<IThemeProvider> theme_;
  raw_ptr<ISettingsProvider> provider_;

  std::vector<std::string> categories_;
  std::vector<const SettingDef*> current_settings_;
  size_t selected_category_ = 0;
  int hovered_category_ = -1;
  int hovered_setting_ = -1;
  int hovered_toggle_ = -1;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
