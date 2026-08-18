// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"

#include "ui/views/controls/webview/webview.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// PdfContentView
// ─────────────────────────────────────────────────────────────────────────────
// Renders PDF documents using Chromium's built-in PDF viewer (PDFium).
// Embedded as a child view within the browser content area.

class PdfContentView : public views::View {
 public:
  METADATA_HEADER(PdfContentView)

  explicit PdfContentView(IThemeProvider* theme);
  ~PdfContentView() override;

  // Load a PDF from URL.
  void LoadPdf(const GURL& url);

  // Navigation
  void GoToPage(int page);
  void NextPage();
  void PreviousPage();
  int GetCurrentPage() const;
  int GetTotalPages() const;

  // Zoom
  void SetZoom(double zoom);
  double GetZoom() const;
  void ZoomIn();
  void ZoomOut();
  void ResetZoom();

  // Download
  void DownloadPdf();

  // Print
  void PrintPdf();

  // View overrides
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  void InitToolbar();
  void UpdateToolbar();

  raw_ptr<IThemeProvider> theme_;
  std::unique_ptr<views::WebView> web_view_;

  // Toolbar
  raw_ptr<views::View> toolbar_= nullptr;
  raw_ptr<views::Label> page_label_= nullptr;
  raw_ptr<views::LabelButton> prev_btn_= nullptr;
  raw_ptr<views::LabelButton> next_btn_= nullptr;
  raw_ptr<views::LabelButton> zoom_out_btn_= nullptr;
  raw_ptr<views::LabelButton> zoom_in_btn_= nullptr;
  raw_ptr<views::LabelButton> download_btn_= nullptr;
  raw_ptr<views::LabelButton> print_btn_= nullptr;

  GURL current_url_;
  int current_page_ = 1;
  int total_pages_ = 1;
  double zoom_ = 1.0;

  base::WeakPtrFactory<PdfContentView> weak_factory_{this};
};

}  // namespace veor
