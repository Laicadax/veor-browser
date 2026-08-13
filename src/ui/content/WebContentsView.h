// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/mojom/frame/find_in_page.mojom.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"

namespace veor {

class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// WebContentsView
// ─────────────────────────────────────────────────────────────────────────────
// Wraps content::WebContents inside a views::WebView.
// Observes WebContents events and translates them into VEOR callbacks.
//
// Replaces ContentViewImpl (placeholder) with real web rendering.
// ─────────────────────────────────────────────────────────────────────────────

class WebContentsView : public views::WebView,
                        public content::WebContentsObserver {
  METADATA_HEADER(WebContentsView, views::WebView)

 public:
  WebContentsView(content::BrowserContext* browser_context,
                  IThemeProvider* theme);
  ~WebContentsView() override;

  // Navigation
  void LoadUrl(const GURL& url);
  void Reload();
  void Stop();
  GURL GetCurrentUrl() const;
  bool IsLoading() const;

  bool CanGoBack() const;
  bool CanGoForward() const;
  void GoBack();
  void GoForward();

  // Callbacks
  void SetOnTitleChanged(base::RepeatingCallback<void(const std::string&)> cb);
  void SetOnUrlChanged(base::RepeatingCallback<void(const GURL&)> cb);
  void SetOnLoadingStateChanged(base::RepeatingCallback<void(bool)> cb);
  void SetOnSecurityStateChanged(
      base::RepeatingCallback<void(bool secure, bool mixed)> cb);

  // Find in page
  void Find(const std::string& text, bool forward, bool match_case);
  void StopFinding();
  void SetOnFindResult(base::RepeatingCallback<void(int active_match,
                                                      int matches)> cb);

  // Zoom
  void ZoomIn();
  void ZoomOut();
  void ResetZoom();

  // Fullscreen
  void EnterFullscreen();
  void ExitFullscreen();

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidChangeVisibleSecurityState() override;
  void FindReply(int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;

 private:
  std::unique_ptr<content::WebContents> web_contents_;
  IThemeProvider* theme_ = nullptr;

  base::RepeatingCallback<void(const std::string&)> on_title_changed_;
  base::RepeatingCallback<void(const GURL&)> on_url_changed_;
  base::RepeatingCallback<void(bool)> on_loading_changed_;
  base::RepeatingCallback<void(bool secure, bool mixed)> on_security_changed_;
  base::RepeatingCallback<void(int active_match, int matches)> on_find_result_;

  int find_request_id_ = 0;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
