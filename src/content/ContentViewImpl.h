// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include "content/IContentView.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// ContentViewImpl
// ─────────────────────────────────────────────────────────────────────────────
// Placeholder content view. Displays URL text and loading state.
// Will be replaced with WebContentsView when Content API is integrated.
// ─────────────────────────────────────────────────────────────────────────────

class ContentViewImpl : public views::View, public IContentView {
  METADATA_HEADER(ContentViewImpl, views::View)

 public:
  explicit ContentViewImpl(IThemeProvider* theme);
  ~ContentViewImpl() override;

  // IContentView
  void LoadUrl(const GURL& url) override;
  void Reload() override;
  void Stop() override;
  GURL GetCurrentUrl() const override;
  bool IsLoading() const override;
  void SetOnTitleChanged(base::RepeatingCallback<void(const std::string&)> cb) override;
  void SetOnUrlChanged(base::RepeatingCallback<void(const GURL&)> cb) override;
  void SetOnLoadingStateChanged(base::RepeatingCallback<void(bool)> cb) override;

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  void OnLoadCompleted(const GURL& url);

  IThemeProvider* theme_ = nullptr;
  GURL current_url_;
  std::string title_;
  bool loading_ = false;

  base::RepeatingCallback<void(const std::string&)> on_title_changed_;
  base::RepeatingCallback<void(const GURL&)> on_url_changed_;
  base::RepeatingCallback<void(bool)> on_loading_changed_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ContentViewImpl> weak_factory_{this};
};

}  // namespace veor
