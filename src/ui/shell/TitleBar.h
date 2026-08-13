// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/shell/ITitleBar.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;
class OmniboxView;

class TitleBar : public views::View, public ITitleBar {
  METADATA_HEADER(TitleBar, views::View)

 public:
  explicit TitleBar(IThemeProvider* theme);
  ~TitleBar() override;

  OmniboxView* GetOmnibox() { return omnibox_; }

  // ITitleBar
  void SetCanGoBack(bool can) override;
  void SetCanGoForward(bool can) override;
  void SetWorkspaceName(const std::string& name) override;
  void SetWorkspaceList(const std::vector<std::string>& names,
                        size_t active_index) override;
  void SetOnBackPressed(base::RepeatingClosure cb) override;
  void SetOnForwardPressed(base::RepeatingClosure cb) override;
  void SetOnWorkspaceSelected(base::RepeatingCallback<void(size_t)> cb) override;
  void SetOnCommandPaletteRequested(base::RepeatingClosure cb) override;
  void SetOnCloseRequested(base::RepeatingClosure cb) override;

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  enum class HoveredButton { kNone, kBack, kForward, kPalette, kClose };
  IThemeProvider* theme_ = nullptr;

  views::Label* brand_label_ = nullptr;
  views::Label* workspace_label_ = nullptr;
  OmniboxView* omnibox_ = nullptr;

  views::View* back_button_ = nullptr;
  views::View* forward_button_ = nullptr;
  views::View* palette_button_ = nullptr;
  views::View* close_button_ = nullptr;

  bool can_go_back_ = false;
  bool can_go_forward_ = false;
  std::vector<std::string> workspace_names_;
  size_t active_workspace_index_ = 0;

  base::RepeatingClosure on_back_;
  base::RepeatingClosure on_forward_;
  base::RepeatingCallback<void(size_t)> on_workspace_selected_;
  base::RepeatingClosure on_command_palette_;
  base::RepeatingClosure on_close_;

  HoveredButton hovered_button_ = HoveredButton::kNone;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
