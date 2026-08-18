// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include "ui/base/metadata/metadata_header_macros.h"
#include "base/memory/raw_ptr.h"
#include "ui/omnibox/IOmnibox.h"
#include "ui/views/controls/textfield/textfield.h"

namespace veor {

class IThemeProvider;

class OmniboxView : public views::Textfield, public IOmnibox {
  METADATA_HEADER(OmniboxView, views::Textfield)

 public:
  explicit OmniboxView(IThemeProvider* theme);
  ~OmniboxView() override;

  void SetText(const std::string& text) override;
  std::string GetText() const override;
  void SetSecure(bool secure, bool mixed) override;
  void SetLoading(bool loading) override;
  void Focus() override;
  void Blur() override;
  void SetOnCommit(base::RepeatingCallback<void(const std::string&)> cb) override;
  void SetOnFocusChanged(base::RepeatingCallback<void(bool)> cb) override;

  void OnPaintBorder(gfx::Canvas* canvas) override;
  void OnFocus() override;
  void OnBlur() override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  raw_ptr<IThemeProvider> theme_= nullptr;
  bool secure_ = false;
  bool mixed_ = false;
  bool loading_ = false;

  base::RepeatingCallback<void(const std::string&)> on_commit_;
  base::RepeatingCallback<void(bool)> on_focus_changed_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
