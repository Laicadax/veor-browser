// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include "ui/base/metadata/metadata_header_macros.h"
#include "base/memory/raw_ptr.h"
#include "ui/ntp/INewTabPage.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

class NewTabPageView : public views::View, public INewTabPage {
  METADATA_HEADER(NewTabPageView, views::View)

 public:
  explicit NewTabPageView(IThemeProvider* theme);
  ~NewTabPageView() override;

  // INewTabPage
  void Show() override;
  void Hide() override;

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  raw_ptr<IThemeProvider> theme_= nullptr;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
