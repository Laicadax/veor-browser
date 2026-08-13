// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "content/ContentViewImpl.h"

#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(ContentViewImpl)
END_METADATA

ContentViewImpl::ContentViewImpl(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetBackground(
      views::CreateSolidBackground(theme_->GetColor(ColorRole::kVoid)));
}

ContentViewImpl::~ContentViewImpl() = default;

void ContentViewImpl::LoadUrl(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  current_url_ = url;
  loading_ = true;

  if (on_url_changed_)
    on_url_changed_.Run(url);
  if (on_loading_changed_)
    on_loading_changed_.Run(true);

  // Async load simulation
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ContentViewImpl::OnLoadCompleted,
                     weak_factory_.GetWeakPtr(), url),
      base::Milliseconds(800));
  SchedulePaint();
}

void ContentViewImpl::OnLoadCompleted(const GURL& url) {
  if (url != current_url_)
    return;
  loading_ = false;
  title_ = url.host();
  if (on_title_changed_)
    on_title_changed_.Run(title_);
  if (on_loading_changed_)
    on_loading_changed_.Run(false);
  SchedulePaint();
}

void ContentViewImpl::Reload() {
  if (current_url_.is_valid())
    LoadUrl(current_url_);
}

void ContentViewImpl::Stop() {
  loading_ = false;
  if (on_loading_changed_)
    on_loading_changed_.Run(false);
  SchedulePaint();
}

GURL ContentViewImpl::GetCurrentUrl() const {
  return current_url_;
}

bool ContentViewImpl::IsLoading() const {
  return loading_;
}

void ContentViewImpl::SetOnTitleChanged(
    base::RepeatingCallback<void(const std::string&)> cb) {
  on_title_changed_ = std::move(cb);
}

void ContentViewImpl::SetOnUrlChanged(
    base::RepeatingCallback<void(const GURL&)> cb) {
  on_url_changed_ = std::move(cb);
}

void ContentViewImpl::SetOnLoadingStateChanged(
    base::RepeatingCallback<void(bool)> cb) {
  on_loading_changed_ = std::move(cb);
}

void ContentViewImpl::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  canvas->FillRect(GetLocalBounds(), theme_->GetColor(ColorRole::kVoid));

  if (!current_url_.is_valid()) {
    // Empty state — nothing to show
    return;
  }

  int cx = width() / 2;
  int cy = height() / 2;

  // URL — centered, muted
  gfx::FontList url_font("Inter, 13px");
  std::u16string url_text = base::UTF8ToUTF16(current_url_.spec());
  int url_width = canvas->GetStringWidth(url_text, url_font);

  gfx::Rect url_rect(cx - url_width / 2, cy - 10,
                     url_width, 20);

  canvas->DrawStringRect(
      url_text, url_rect, url_font,
      gfx::Canvas::TEXT_ALIGN_CENTER,
      theme_->GetColor(ColorRole::kTextSecondary));

  // Loading indicator — thin pulsing line at top
  if (loading_) {
    SkPaint load_paint;
    load_paint.setColor(theme_->GetColor(ColorRole::kCrimsonGlow));
    load_paint.setStrokeWidth(2);
    canvas->sk_canvas()->drawLine(0, 1, width(), 1, load_paint);
  }
}

}  // namespace veor
