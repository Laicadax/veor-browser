// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/downloads/DownloadsView.h"

#include "downloads/IDownloadController.h"
#include "ui/theme/IThemeProvider.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/views/background.h"

namespace veor {

BEGIN_METADATA(DownloadsView)
END_METADATA

namespace {

constexpr int kItemHeight = 48;
constexpr int kProgressBarHeight = 2;
constexpr int kPadding = 16;
constexpr int kButtonWidth = 48;

SkColor StateColor(DownloadState state, IThemeProvider* theme) {
  switch (state) {
    case DownloadState::kCompleted:
      return theme->GetColor(ColorRole::kAccentCrimson);
    case DownloadState::kInProgress:
      return theme->GetColor(ColorRole::kAccentCrimson);
    case DownloadState::kPaused:
      return theme->GetColor(ColorRole::kTextQuaternary);
    case DownloadState::kFailed:
      return theme->GetColor(ColorRole::kTextQuaternary);
    case DownloadState::kCancelled:
      return theme->GetColor(ColorRole::kEdge);
    default:
      return theme->GetColor(ColorRole::kTextQuaternary);
  }
}

std::string StateLabel(DownloadState state) {
  switch (state) {
    case DownloadState::kPending: return "Pending";
    case DownloadState::kInProgress: return "Downloading";
    case DownloadState::kPaused: return "Paused";
    case DownloadState::kCompleted: return "Done";
    case DownloadState::kFailed: return "Failed";
    case DownloadState::kCancelled: return "Cancelled";
  }
  return "";
}

}  // namespace

DownloadsView::DownloadsView(IThemeProvider* theme,
                             IDownloadController* controller)
    : theme_(theme), controller_(controller) {
  DCHECK(theme_);
  SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurface)));
}

DownloadsView::~DownloadsView() = default;

void DownloadsView::Show() {
  SetVisible(true);
  Reload();
  SchedulePaint();
}

void DownloadsView::Hide() {
  SetVisible(false);
  SchedulePaint();
}

bool DownloadsView::IsVisible() const {
  return views::View::GetVisible();
}

void DownloadsView::Reload() {
  if (controller_)
    items_ = controller_->GetAllDownloads();
  SchedulePaint();
}

gfx::Size DownloadsView::CalculatePreferredSize() const {
  return gfx::Size(480, std::max(200, static_cast<int>(items_.size()) * kItemHeight + kPadding * 2));
}

void DownloadsView::Layout() {
  // Single-column list fills the view
}

void DownloadsView::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  gfx::Rect bounds = GetLocalBounds();
  canvas->FillRect(bounds, theme_->GetColor(ColorRole::kSurface));

  // Top edge — thin crimson line
  SkPaint edge_paint;
  edge_paint.setColor(theme_->GetColor(ColorRole::kAccentCrimson));
  edge_paint.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(0, 0, bounds.width(), 0, edge_paint);

  gfx::FontList title_font("Inter, 12px");
  gfx::FontList detail_font("Inter, 11px");
  SkColor text_color = theme_->GetColor(ColorRole::kTextPrimary);
  SkColor muted = theme_->GetColor(ColorRole::kTextQuaternary);

  int y = kPadding;

  // Header
  canvas->DrawStringRect(u"Downloads",
                         gfx::Rect(kPadding, y, bounds.width() - kPadding * 2, 20),
                         title_font, gfx::Canvas::TEXT_ALIGN_LEFT, text_color);
  y += 28;

  // Separator
  SkPaint sep;
  sep.setColor(theme_->GetColor(ColorRole::kEdge));
  sep.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(kPadding, y, bounds.width() - kPadding, y, sep);
  y += 12;

  if (items_.empty()) {
    canvas->DrawStringRect(u"No downloads",
                           gfx::Rect(kPadding, y, bounds.width() - kPadding * 2, 16),
                           detail_font, gfx::Canvas::TEXT_ALIGN_LEFT, muted);
    return;
  }

  for (size_t i = 0; i < items_.size(); ++i) {
    const auto& item = items_[i];
    int item_y = y + static_cast<int>(i) * kItemHeight;

    // Hover background
    if (static_cast<int>(i) == hovered_item_) {
      SkPaint hover;
      hover.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
      canvas->DrawRect(gfx::Rect(0, item_y, bounds.width(), kItemHeight), hover);
    }

    // Filename
    std::u16string filename = base::UTF8ToUTF16(item.filename);
    canvas->DrawStringRect(filename,
                           gfx::Rect(kPadding, item_y + 4,
                                     bounds.width() - kPadding * 2 - kButtonWidth, 16),
                           title_font, gfx::Canvas::TEXT_ALIGN_LEFT, text_color);

    // State label
    std::u16string state = base::UTF8ToUTF16(StateLabel(item.state));
    canvas->DrawStringRect(state,
                           gfx::Rect(kPadding, item_y + 22,
                                     80, 14),
                           detail_font, gfx::Canvas::TEXT_ALIGN_LEFT,
                           StateColor(item.state, theme_));

    // Size / progress
    std::string size_text;
    if (item.total_bytes > 0) {
      size_text = std::to_string(item.progress_percent) + "% of " +
                  std::to_string(item.total_bytes / 1024) + " KB";
    } else {
      size_text = std::to_string(item.received_bytes / 1024) + " KB";
    }
    canvas->DrawStringRect(base::UTF8ToUTF16(size_text),
                           gfx::Rect(kPadding + 90, item_y + 22,
                                     150, 14),
                           detail_font, gfx::Canvas::TEXT_ALIGN_LEFT, muted);

    // Progress bar
    if (item.state == DownloadState::kInProgress ||
        item.state == DownloadState::kPaused) {
      SkPaint bg;
      bg.setColor(theme_->GetColor(ColorRole::kEdge));
      canvas->DrawRect(gfx::Rect(kPadding, item_y + 40,
                                 bounds.width() - kPadding * 2 - kButtonWidth,
                                 kProgressBarHeight), bg);

      if (item.total_bytes > 0) {
        int progress_width = (bounds.width() - kPadding * 2 - kButtonWidth) *
                             item.progress_percent / 100;
        SkPaint fg;
        fg.setColor(theme_->GetColor(ColorRole::kAccentCrimson));
        canvas->DrawRect(gfx::Rect(kPadding, item_y + 40,
                                   progress_width, kProgressBarHeight), fg);
      }
    }

    // Action button area (right side)
    int btn_x = bounds.width() - kButtonWidth - 8;
    if (item.state == DownloadState::kInProgress) {
      // Pause button
      SkPaint btn_bg;
      btn_bg.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
      canvas->DrawRect(gfx::Rect(btn_x, item_y + 10, kButtonWidth, 28), btn_bg);
      canvas->DrawStringRect(u"||",
                             gfx::Rect(btn_x, item_y + 10, kButtonWidth, 28),
                             detail_font, gfx::Canvas::TEXT_ALIGN_CENTER, text_color);
    } else if (item.state == DownloadState::kPaused) {
      // Resume button
      SkPaint btn_bg;
      btn_bg.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
      canvas->DrawRect(gfx::Rect(btn_x, item_y + 10, kButtonWidth, 28), btn_bg);
      canvas->DrawStringRect(u"\u25B6",
                             gfx::Rect(btn_x, item_y + 10, kButtonWidth, 28),
                             detail_font, gfx::Canvas::TEXT_ALIGN_CENTER, text_color);
    } else if (item.state == DownloadState::kCompleted) {
      // Open button
      SkPaint btn_bg;
      btn_bg.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
      canvas->DrawRect(gfx::Rect(btn_x, item_y + 10, kButtonWidth, 28), btn_bg);
      canvas->DrawStringRect(u"Open",
                             gfx::Rect(btn_x, item_y + 10, kButtonWidth, 28),
                             detail_font, gfx::Canvas::TEXT_ALIGN_CENTER, text_color);
    }

    // Cancel/Remove (×) for all except completed
    if (item.state != DownloadState::kCompleted) {
      canvas->DrawStringRect(u"\u00D7",
                             gfx::Rect(bounds.width() - 24, item_y + 4, 16, 16),
                             gfx::FontList("Inter, 12px"),
                             gfx::Canvas::TEXT_ALIGN_CENTER,
                             theme_->GetColor(ColorRole::kAccentCrimson));
    }
  }
}

bool DownloadsView::OnMousePressed(const ui::MouseEvent& event) {
  int y = event.y();
  int header_height = kPadding + 28 + 12;

  if (y < header_height)
    return true;

  int item_index = (y - header_height) / kItemHeight;
  if (item_index < 0 || item_index >= static_cast<int>(items_.size()))
    return true;

  int btn_x = width() - kButtonWidth - 8;
  int item_y = header_height + item_index * kItemHeight;

  // Check action button click
  if (event.x() >= btn_x && event.x() < btn_x + kButtonWidth &&
      event.y() >= item_y + 10 && event.y() < item_y + 38) {
    const auto& item = items_[item_index];
    if (controller_) {
      if (item.state == DownloadState::kInProgress) {
        controller_->PauseDownload(item.id);
      } else if (item.state == DownloadState::kPaused) {
        controller_->ResumeDownload(item.id);
      }
    }
    Reload();
    return true;
  }

  // Check cancel (×) click
  if (event.x() >= width() - 24 && event.x() < width() - 8 &&
      event.y() >= item_y + 4 && event.y() < item_y + 20) {
    if (controller_) {
      controller_->CancelDownload(items_[item_index].id);
    }
    Reload();
    return true;
  }

  return true;
}

}  // namespace veor
