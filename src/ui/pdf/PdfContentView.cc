// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/pdf/PdfContentView.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/views_delegate.h"

#include "ui/theme/IThemeProvider.h"
#include "core/logging/VeorLogger.h"

namespace veor {

BEGIN_METADATA(PdfContentView)
END_METADATA

PdfContentView::PdfContentView(IThemeProvider* theme) : theme_(theme) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  InitToolbar();

  web_view_ = std::make_unique<views::WebView>(nullptr);
  web_view_->set_owned_by_client();
  AddChildView(web_view_.get());

  SetBackground(views::CreateSolidBackground(theme_->GetColor(ColorRole::kSurfacePrimary)));
}

PdfContentView::~PdfContentView() = default;

void PdfContentView::InitToolbar() {
  toolbar_ = AddChildView(std::make_unique<views::View>());
  toolbar_->SetLayoutManager(std::make_unique<views::FlexLayout>()
      .SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter));
  toolbar_->SetPreferredSize(gfx::Size(0, 40));
  toolbar_->SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurfaceSecondary)));
  toolbar_->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, theme_->GetColor(ColorRole::kBorderPrimary)));

  auto make_btn = [this](const std::string& text) {
    auto* btn = toolbar_->AddChildView(
        std::make_unique<views::LabelButton>(
            views::Button::PressedCallback(), base::UTF8ToUTF16(text)));
    btn->SetEnabledTextColors(theme_->GetColor(ColorRole::kTextSecondary));
    btn->SetBackground(views::CreateSolidBackground(SK_ColorTRANSPARENT));
    return btn;
  };

  prev_btn_ = make_btn("<");
  prev_btn_->SetCallback(base::BindRepeating(&PdfContentView::PreviousPage,
                                              base::Unretained(this)));

  page_label_ = toolbar_->AddChildView(std::make_unique<views::Label>());
  page_label_->SetEnabledColor(theme_->GetColor(ColorRole::kTextSecondary));

  next_btn_ = make_btn(">");
  next_btn_->SetCallback(base::BindRepeating(&PdfContentView::NextPage,
                                              base::Unretained(this)));

  toolbar_->AddChildView(std::make_unique<views::View>())
      ->SetPreferredSize(gfx::Size(24, 1));

  zoom_out_btn_ = make_btn("-");
  zoom_out_btn_->SetCallback(base::BindRepeating(&PdfContentView::ZoomOut,
                                                  base::Unretained(this)));

  zoom_in_btn_ = make_btn("+");
  zoom_in_btn_->SetCallback(base::BindRepeating(&PdfContentView::ZoomIn,
                                                 base::Unretained(this)));

  toolbar_->AddChildView(std::make_unique<views::View>())
      ->SetPreferredSize(gfx::Size(24, 1));

  download_btn_ = make_btn("Download");
  download_btn_->SetCallback(base::BindRepeating(&PdfContentView::DownloadPdf,
                                                  base::Unretained(this)));

  print_btn_ = make_btn("Print");
  print_btn_->SetCallback(base::BindRepeating(&PdfContentView::PrintPdf,
                                               base::Unretained(this)));

  UpdateToolbar();
}

void PdfContentView::LoadPdf(const GURL& url) {
  current_url_ = url;
  if (web_view_->GetWebContents()) {
    web_view_->GetWebContents()->GetController().LoadURL(
        url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
        std::string());
  }
  VEOR_LOGI(LogCategory::kContent, "Loading PDF: " + url.spec());
}

void PdfContentView::GoToPage(int page) {
  current_page_ = std::max(1, std::min(page, total_pages_));
  UpdateToolbar();
  // TODO: Send JavaScript message to PDF viewer to navigate to page
}

void PdfContentView::NextPage() {
  GoToPage(current_page_ + 1);
}

void PdfContentView::PreviousPage() {
  GoToPage(current_page_ - 1);
}

int PdfContentView::GetCurrentPage() const {
  return current_page_;
}

int PdfContentView::GetTotalPages() const {
  return total_pages_;
}

void PdfContentView::SetZoom(double zoom) {
  zoom_ = std::max(0.25, std::min(5.0, zoom));
  UpdateToolbar();
  // TODO: Send JavaScript message to PDF viewer to set zoom
}

double PdfContentView::GetZoom() const {
  return zoom_;
}

void PdfContentView::ZoomIn() {
  SetZoom(zoom_ * 1.25);
}

void PdfContentView::ZoomOut() {
  SetZoom(zoom_ / 1.25);
}

void PdfContentView::ResetZoom() {
  SetZoom(1.0);
}

void PdfContentView::DownloadPdf() {
  if (!current_url_.is_empty()) {
    // TODO: Trigger download via DownloadController
    VEOR_LOGI(LogCategory::kContent,
              "PDF download requested: " + current_url_.spec());
  }
}

void PdfContentView::PrintPdf() {
  if (web_view_->GetWebContents()) {
    web_view_->GetWebContents()->Print();
  }
}

void PdfContentView::UpdateToolbar() {
  page_label_->SetText(base::UTF8ToUTF16(
      base::NumberToString(current_page_) + " / " +
      base::NumberToString(total_pages_)));
  prev_btn_->SetEnabled(current_page_ > 1);
  next_btn_->SetEnabled(current_page_ < total_pages_);
}

void PdfContentView::Layout() {
  views::View::Layout();
  if (toolbar_ && web_view_) {
    int toolbar_h = toolbar_->GetPreferredSize().height();
    toolbar_->SetBounds(0, 0, width(), toolbar_h);
    web_view_->SetBounds(0, toolbar_h, width(), height() - toolbar_h);
  }
}

gfx::Size PdfContentView::CalculatePreferredSize() const {
  return gfx::Size(800, 600);
}

}  // namespace veor
