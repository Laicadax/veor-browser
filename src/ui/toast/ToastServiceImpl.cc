// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/toast/ToastServiceImpl.h"

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/theme/IThemeProvider.h"
#include "ui/views/background.h"

namespace veor {

namespace {

constexpr int kToastWidth = 280;
constexpr int kToastHeight = 56;
constexpr int kToastMargin = 12;

class ToastView : public views::View {
 public:
  ToastView(IThemeProvider* theme, const ToastData& data)
      : theme_(theme), data_(data) {
    SetPreferredSize(gfx::Size(kToastWidth, kToastHeight));
    SetBackground(views::CreateSolidBackground(
        theme_->GetColor(ColorRole::kOverlayBackground)));
  }

  void OnPaint(gfx::Canvas* canvas) override {
    gfx::Rect bounds = GetLocalBounds();

    // Thin border
    SkPaint border;
    border.setColor(theme_->GetColor(ColorRole::kToastBorder));
    border.setStrokeWidth(1);
    border.setStyle(SkPaint::kStroke_Style);
    canvas->sk_canvas()->drawRect(
        SkRect::MakeXYWH(0.5f, 0.5f, bounds.width() - 1, bounds.height() - 1),
        border);

    // Title
    gfx::FontList title_font("Inter, 11px");
    canvas->DrawStringRect(
        base::UTF8ToUTF16(data_.title),
        gfx::Rect(12, 8, bounds.width() - 24, 16),
        title_font, gfx::Canvas::TEXT_ALIGN_LEFT,
        theme_->GetColor(ColorRole::kTextPrimary));

    // Detail
    if (!data_.detail.empty()) {
      gfx::FontList detail_font("Inter, 10px");
      canvas->DrawStringRect(
          base::UTF8ToUTF16(data_.detail),
          gfx::Rect(12, 26, bounds.width() - 24, 14),
          detail_font, gfx::Canvas::TEXT_ALIGN_LEFT,
          theme_->GetColor(ColorRole::kTextTertiary));
    }

    // Progress bar at bottom
    SkPaint progress;
    progress.setColor(theme_->GetColor(ColorRole::kToastProgressBar));
    progress.setStrokeWidth(2);
    float elapsed = (base::TimeTicks::Now() - start_time_).InMillisecondsF();
    float pct = std::clamp(1.0f - elapsed / data_.duration_ms, 0.0f, 1.0f);
    float bar_w = (bounds.width() - 2) * pct;
    canvas->sk_canvas()->drawLine(1, bounds.height() - 1,
                                  1 + bar_w, bounds.height() - 1, progress);
  }

  void SetStartTime(base::TimeTicks t) { start_time_ = t; }

 private:
  IThemeProvider* theme_;
  ToastData data_;
  base::TimeTicks start_time_;
};

}  // namespace

ToastServiceImpl::ToastServiceImpl(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
}

ToastServiceImpl::~ToastServiceImpl() = default;

void ToastServiceImpl::SetContainer(views::View* container) {
  container_ = container;
}

void ToastServiceImpl::Show(const ToastData& toast) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!container_) return;

  int64_t toast_id = next_toast_id_++;

  auto active = std::make_unique<ActiveToast>();
  active->id = toast_id;
  active->data = toast;
  active->view = container_->AddChildView(
      std::make_unique<ToastView>(theme_, toast));
  active->start_time = base::TimeTicks::Now();

  static_cast<ToastView*>(active->view)->SetStartTime(active->start_time);

  active->timer.Start(
      FROM_HERE, base::Milliseconds(toast.duration_ms),
      base::BindOnce(&ToastServiceImpl::DismissToastById,
                     weak_factory_.GetWeakPtr(), toast_id));

  toasts_.push_back(std::move(active));
  LayoutToasts();
}

void ToastServiceImpl::DismissAll() {
  for (auto& t : toasts_) {
    t->timer.Stop();
    if (t->view && container_)
      container_->RemoveChildViewT(t->view);
  }
  toasts_.clear();
}

void ToastServiceImpl::LayoutToasts() {
  if (!container_) return;

  int parent_w = container_->width();
  int parent_h = container_->height();
  int y = parent_h - kToastMargin;

  for (auto it = toasts_.rbegin(); it != toasts_.rend(); ++it) {
    y -= kToastHeight + kToastMargin;
    (*it)->view->SetBounds(parent_w - kToastWidth - kToastMargin, y,
                           kToastWidth, kToastHeight);
  }
}

void ToastServiceImpl::DismissToastById(int64_t id) {
  auto it = std::find_if(toasts_.begin(), toasts_.end(),
                         [id](const auto& t) { return t->id == id; });
  if (it == toasts_.end()) return;

  auto* view = (*it)->view;
  (*it)->timer.Stop();

  if (container_ && view)
    container_->RemoveChildViewT(view);

  toasts_.erase(it);
  LayoutToasts();
}

}  // namespace veor
