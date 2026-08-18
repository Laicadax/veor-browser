// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>
#include "base/memory/raw_ptr.h"

#include "base/functional/callback.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;
class OmniboxView;

// ─────────────────────────────────────────────────────────────────────────────
// FindBarView
// ─────────────────────────────────────────────────────────────────────────────
// Thin find bar anchored at the bottom of the content area.
//
// No rounded corners. No gradients. Just a line, text, and arrows.
// ─────────────────────────────────────────────────────────────────────────────

class FindBarView : public views::View {
  METADATA_HEADER(FindBarView, views::View)

 public:
  explicit FindBarView(IThemeProvider* theme);
  ~FindBarView() override;

  void Show();
  void Hide();
  bool IsVisible() const;

  void SetMatchCount(int active_match, int total_matches);

  // Callbacks
  void SetOnFindRequested(
      base::RepeatingCallback<void(const std::string& text,
                                   bool forward,
                                   bool match_case)> cb);
  void SetOnCloseRequested(base::RepeatingClosure cb);

  void FocusSearchBox();

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;

 private:
  void OnSearchTextChanged();
  void OnPrevClicked();
  void OnNextClicked();
  void OnCloseClicked();

  raw_ptr<IThemeProvider> theme_;

  raw_ptr<views::View> search_box_= nullptr;
  raw_ptr<views::View> prev_button_= nullptr;
  raw_ptr<views::View> next_button_= nullptr;
  raw_ptr<views::View> close_button_= nullptr;

  std::u16string match_text_;
  std::u16string search_text_;
  bool match_case_ = false;

  base::RepeatingCallback<void(const std::string& text,
                               bool forward,
                               bool match_case)> on_find_;
  base::RepeatingClosure on_close_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
