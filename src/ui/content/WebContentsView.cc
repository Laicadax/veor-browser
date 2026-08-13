// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/content/WebContentsView.h"

#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/find_request_manager.h"
#include "ui/views/controls/webview/webview.h"

#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(WebContentsView)
END_METADATA

WebContentsView::WebContentsView(content::BrowserContext* browser_context,
                                 IThemeProvider* theme)
    : views::WebView(browser_context), theme_(theme) {
  DCHECK(browser_context);

  content::WebContents::CreateParams params(browser_context);
  web_contents_ = content::WebContents::Create(params);
  DCHECK(web_contents_);

  SetWebContents(web_contents_.get());
  Observe(web_contents_.get());
}

WebContentsView::~WebContentsView() = default;

void WebContentsView::LoadUrl(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!url.is_valid()) return;

  web_contents_->GetController().LoadURL(
      url, content::Referrer(), ui::PAGE_TRANSITION_TYPED, std::string());
}

void WebContentsView::Reload() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents_->GetController().Reload(content::ReloadType::NORMAL, true);
}

void WebContentsView::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents_->Stop();
}

GURL WebContentsView::GetCurrentUrl() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  return entry ? entry->GetURL() : GURL();
}

bool WebContentsView::IsLoading() const {
  return web_contents_->IsLoading();
}

bool WebContentsView::CanGoBack() const {
  return web_contents_->GetController().CanGoBack();
}

bool WebContentsView::CanGoForward() const {
  return web_contents_->GetController().CanGoForward();
}

void WebContentsView::GoBack() {
  web_contents_->GetController().GoBack();
}

void WebContentsView::GoForward() {
  web_contents_->GetController().GoForward();
}

void WebContentsView::SetOnTitleChanged(
    base::RepeatingCallback<void(const std::string&)> cb) {
  on_title_changed_ = std::move(cb);
}

void WebContentsView::SetOnUrlChanged(
    base::RepeatingCallback<void(const GURL&)> cb) {
  on_url_changed_ = std::move(cb);
}

void WebContentsView::SetOnLoadingStateChanged(
    base::RepeatingCallback<void(bool)> cb) {
  on_loading_changed_ = std::move(cb);
}

void WebContentsView::SetOnSecurityStateChanged(
    base::RepeatingCallback<void(bool secure, bool mixed)> cb) {
  on_security_changed_ = std::move(cb);
}

// ── WebContentsObserver ──────────────────────────────────────────────────────

void WebContentsView::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  GURL url = navigation_handle->GetURL();
  if (on_url_changed_)
    on_url_changed_.Run(url);

  // Security state
  if (on_security_changed_) {
    bool secure = url.SchemeIsCryptographic();
    bool mixed = false;
    if (secure && web_contents_->GetController().GetVisibleEntry()) {
      const auto& ssl = web_contents_->GetController().GetVisibleEntry()->GetSSL();
      mixed = ssl.content_status &
              (net::SSLStatus::RAN_INSECURE_CONTENT |
               net::SSLStatus::DISPLAYED_INSECURE_CONTENT);
    }
    on_security_changed_.Run(secure, mixed);
  }
}

void WebContentsView::DidStartLoading() {
  if (on_loading_changed_)
    on_loading_changed_.Run(true);
}

void WebContentsView::DidStopLoading() {
  if (on_loading_changed_)
    on_loading_changed_.Run(false);
}

void WebContentsView::TitleWasSet(content::NavigationEntry* entry) {
  if (on_title_changed_)
    on_title_changed_.Run(base::UTF16ToUTF8(web_contents_->GetTitle()));
}

void WebContentsView::DidChangeVisibleSecurityState() {
  if (!on_security_changed_)
    return;

  GURL url = GetCurrentUrl();
  bool secure = url.SchemeIsCryptographic();
  bool mixed = false;
  if (secure && web_contents_->GetController().GetVisibleEntry()) {
    const auto& ssl = web_contents_->GetController().GetVisibleEntry()->GetSSL();
    mixed = ssl.content_status &
            (net::SSLStatus::RAN_INSECURE_CONTENT |
             net::SSLStatus::DISPLAYED_INSECURE_CONTENT);
  }
  on_security_changed_.Run(secure, mixed);
}

// ── Find in Page ───────────────────────────────────────────────────────────────

void WebContentsView::Find(const std::string& text,
                           bool forward,
                           bool match_case) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;

  auto* find_manager = content::FindRequestManager::FromWebContents(
      web_contents_.get());
  if (!find_manager)
    return;

  blink::mojom::FindOptionsPtr options = blink::mojom::FindOptions::New();
  options->forward = forward;
  options->match_case = match_case;
  options->new_session = true;

  find_manager->Find(
      base::NumberToString(++find_request_id_),
      base::UTF8ToUTF16(text),
      std::move(options),
      /*wrap_within_frame=*/false);
}

void WebContentsView::StopFinding() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;

  auto* find_manager = content::FindRequestManager::FromWebContents(
      web_contents_.get());
  if (!find_manager)
    return;

  find_manager->StopFinding(content::STOP_FIND_ACTION_CLEAR_SELECTION);
}

void WebContentsView::SetOnFindResult(
    base::RepeatingCallback<void(int active_match, int matches)> cb) {
  on_find_result_ = std::move(cb);
}

void WebContentsView::FindReply(int request_id,
                                int number_of_matches,
                                const gfx::Rect& selection_rect,
                                int active_match_ordinal,
                                bool final_update) {
  if (on_find_result_ && final_update) {
    on_find_result_.Run(active_match_ordinal, number_of_matches);
  }
}

// ── Zoom ───────────────────────────────────────────────────────────────────────

void WebContentsView::ZoomIn() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;
  double level = web_contents_->GetZoomLevel();
  web_contents_->SetZoomLevel(level + 0.5);
}

void WebContentsView::ZoomOut() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;
  double level = web_contents_->GetZoomLevel();
  web_contents_->SetZoomLevel(std::max(level - 0.5, -3.0));
}

void WebContentsView::ResetZoom() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;
  web_contents_->SetZoomLevel(0.0);
}

// ── Fullscreen ─────────────────────────────────────────────────────────────────

void WebContentsView::EnterFullscreen() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;
  web_contents_->EnterFullscreen(
      blink::mojom::FullscreenOptions::New(),
      /*from_embedder=*/nullptr);
}

void WebContentsView::ExitFullscreen() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents_)
    return;
  web_contents_->ExitFullscreen(/*will_cause_resize=*/true);
}

}  // namespace veor
