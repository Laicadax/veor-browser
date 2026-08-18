// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "base/memory/raw_ptr.h"
#include <string>

#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// ReaderModeView
// ─────────────────────────────────────────────────────────────────────────────
// Displays article content in a clean, distraction-free reading format.
// Extracts the main content from HTML using a readability algorithm
// and renders it with VEOR's typography and color scheme.

class ReaderModeView : public views::View {
 public:
  METADATA_HEADER(ReaderModeView)

  explicit ReaderModeView(IThemeProvider* theme);
  ~ReaderModeView() override;

  // Load extracted article content.
  void LoadArticle(const std::string& title,
                   const std::string& byline,
                   const std::string& content_html);

  // Clear content.
  void Clear();

  // View overrides
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  void BuildChrome();

  raw_ptr<IThemeProvider> theme_;

  raw_ptr<views::View> header_= nullptr;
  raw_ptr<views::Label> title_label_= nullptr;
  raw_ptr<views::Label> byline_label_= nullptr;
  raw_ptr<views::ScrollView> scroll_view_= nullptr;
  raw_ptr<views::View> content_container_= nullptr;
  raw_ptr<views::Label> content_label_= nullptr;
};

}  // namespace veor
