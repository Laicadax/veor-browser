// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

class TabStripView : public views::View {
  METADATA_HEADER(TabStripView, views::View)

 public:
  explicit TabStripView(IThemeProvider* theme);
  ~TabStripView() override;

  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  struct TabVisual {
    std::string title;
    std::string url_host;
    bool active = false;
    bool pinned = false;
    bool audio_playing = false;
    SkColor domain_color = SK_ColorTRANSPARENT;
  };

  void SetTabs(const std::vector<TabVisual>& tabs);
  void SetOnTabSelected(base::RepeatingCallback<void(size_t)> cb);
  void SetOnTabClosed(base::RepeatingCallback<void(size_t)> cb);
  void SetOnTabContextMenu(
      base::RepeatingCallback<void(size_t, gfx::Point)> cb);

 private:
  IThemeProvider* theme_ = nullptr;
  std::vector<TabVisual> tabs_;
  int tab_height_ = 36;
  int tab_min_width_ = 80;
  int tab_max_width_ = 200;

  base::RepeatingCallback<void(size_t)> on_tab_selected_;
  base::RepeatingCallback<void(size_t)> on_tab_closed_;
  base::RepeatingCallback<void(size_t, gfx::Point)> on_tab_context_menu_;

  size_t hovered_tab_ = static_cast<size_t>(-1);
  bool hovered_close_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
