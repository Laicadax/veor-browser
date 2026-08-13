// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/reader/ReaderModeView.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/layout/box_layout.h"

#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(ReaderModeView)
END_METADATA

ReaderModeView::ReaderModeView(IThemeProvider* theme) : theme_(theme) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurfacePrimary)));

  BuildChrome();
}

ReaderModeView::~ReaderModeView() = default;

void ReaderModeView::BuildChrome() {
  // Header: title + byline
  header_ = AddChildView(std::make_unique<views::View>());
  header_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(40, 48, 24, 48), 12));
  header_->SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurfacePrimary)));

  title_label_ = header_->AddChildView(std::make_unique<views::Label>());
  title_label_->SetFontList(
      title_label_->font_list().DeriveWithSizeDelta(8)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  title_label_->SetEnabledColor(theme_->GetColor(ColorRole::kTextPrimary));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetMultiLine(true);

  byline_label_ = header_->AddChildView(std::make_unique<views::Label>());
  byline_label_->SetFontList(
      byline_label_->font_list().DeriveWithSizeDelta(-1));
  byline_label_->SetEnabledColor(theme_->GetColor(ColorRole::kTextTertiary));
  byline_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Separator
  auto* separator = AddChildView(std::make_unique<views::View>());
  separator->SetPreferredSize(gfx::Size(0, 1));
  separator->SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kBorderPrimary)));

  // Scrollable content
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetDrawOverflowIndicator(false);

  content_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  content_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(24, 48, 48, 48), 16));
  content_container_->SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurfacePrimary)));

  content_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  content_label_->SetFontList(
      content_label_->font_list().DeriveWithSizeDelta(1));
  content_label_->SetEnabledColor(theme_->GetColor(ColorRole::kTextSecondary));
  content_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  content_label_->SetMultiLine(true);
}

void ReaderModeView::LoadArticle(const std::string& title,
                                 const std::string& byline,
                                 const std::string& content_html) {
  title_label_->SetText(base::UTF8ToUTF16(title));
  byline_label_->SetText(base::UTF8ToUTF16(byline));

  // Strip basic HTML tags for MVP display
  std::string plain = content_html;
  // Remove tags
  size_t pos = 0;
  while ((pos = plain.find('<', pos)) != std::string::npos) {
    size_t end = plain.find('>', pos);
    if (end == std::string::npos) break;
    plain.erase(pos, end - pos + 1);
  }
  // Decode entities
  struct Entity { const char* code; const char* chr; };
  static const Entity entities[] = {
    {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"},
    {"&quot;", "\""}, {"&#39;", "'"}, {"&nbsp;", " "},
  };
  for (const auto& e : entities) {
    pos = 0;
    while ((pos = plain.find(e.code, pos)) != std::string::npos) {
      plain.replace(pos, strlen(e.code), e.chr);
      pos += strlen(e.chr);
    }
  }

  content_label_->SetText(base::UTF8ToUTF16(plain));
  Layout();
}

void ReaderModeView::Clear() {
  title_label_->SetText(std::u16string());
  byline_label_->SetText(std::u16string());
  content_label_->SetText(std::u16string());
}

void ReaderModeView::Layout() {
  views::View::Layout();
  if (header_ && scroll_view_) {
    int header_h = header_->GetPreferredSize().height();
    header_->SetBounds(0, 0, width(), header_h);
    scroll_view_->SetBounds(0, header_h + 1, width(), height() - header_h - 1);
  }
}

gfx::Size ReaderModeView::CalculatePreferredSize() const {
  return gfx::Size(800, 600);
}

}  // namespace veor
