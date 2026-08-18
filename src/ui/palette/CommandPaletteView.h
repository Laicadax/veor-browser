// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>
#include "base/memory/raw_ptr.h"
#include <vector>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/palette/ICommandPalette.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

class CommandPaletteView : public views::View, public ICommandPalette {
  METADATA_HEADER(CommandPaletteView, views::View)

 public:
  explicit CommandPaletteView(IThemeProvider* theme);
  ~CommandPaletteView() override;

  // ICommandPalette
  void Show() override;
  void Hide() override;
  bool IsVisible() const override;
  void SetQuery(const std::string& query) override;
  std::string GetQuery() const override;
  void SetResults(const std::vector<PaletteItem>& items) override;
  void SetSelectedIndex(int index) override;
  void SetOnItemSelected(base::RepeatingCallback<void(int)> cb) override;
  void SetOnQueryChanged(base::RepeatingCallback<void(const std::string&)> cb) override;
  void SetOnDismissed(base::RepeatingClosure cb) override;

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;

 private:
  void DrawBackdrop(gfx::Canvas* canvas);
  void DrawInput(gfx::Canvas* canvas);
  void DrawResults(gfx::Canvas* canvas);

  raw_ptr<IThemeProvider> theme_= nullptr;
  bool visible_ = false;
  std::string query_;
  std::vector<PaletteItem> items_;
  int selected_index_ = -1;

  base::RepeatingCallback<void(int)> on_item_selected_;
  base::RepeatingCallback<void(const std::string&)> on_query_changed_;
  base::RepeatingClosure on_dismissed_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
