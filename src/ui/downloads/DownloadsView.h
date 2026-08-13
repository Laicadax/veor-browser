// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class IDownloadController;
class IThemeProvider;
struct DownloadItem;

// ─────────────────────────────────────────────────────────────────────────────
// DownloadsView
// ─────────────────────────────────────────────────────────────────────────────
// Overlay panel showing active and completed downloads.
//
// No rounded corners. No gradients. Just a list, progress bars, and actions.
// ─────────────────────────────────────────────────────────────────────────────

class DownloadsView : public views::View {
  METADATA_HEADER(DownloadsView, views::View)

 public:
  DownloadsView(IThemeProvider* theme, IDownloadController* controller);
  ~DownloadsView() override;

  void Show();
  void Hide();
  bool IsVisible() const;
  void Reload();

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;

 private:
  void DrawDownloadItem(gfx::Canvas* canvas,
                        const DownloadItem& item,
                        int y,
                        int width);
  bool OnMousePressed(const ui::MouseEvent& event) override;

  IThemeProvider* theme_;
  IDownloadController* controller_;

  std::vector<DownloadItem> items_;
  int hovered_item_ = -1;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
