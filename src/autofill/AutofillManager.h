// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace veor {

class IAutofillStore;
class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// AutofillManager
// ─────────────────────────────────────────────────────────────────────────────
// Per-tab autofill controller. Injects form detection JS, fills credentials,
// and triggers save prompts via toast notifications.
//
// No popups. No modals. Just silent filling and a thin toast for save prompts.
// ─────────────────────────────────────────────────────────────────────────────

class AutofillManager : public content::WebContentsObserver {
 public:
  AutofillManager(content::WebContents* web_contents,
                  IAutofillStore* store,
                  IThemeProvider* theme);
  ~AutofillManager() override;

  // WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // Explicitly trigger a save prompt (called when form submission is detected)
  void PromptSavePassword(const std::string& origin,
                          const std::string& username,
                          const std::string& password);

  // Fill credentials into the current page
  void FillCredentials(const std::string& username,
                       const std::string& password);

  // Check if the current page has login forms and credentials available
  void CheckAndFillForms();

  // Callback for save prompt (set by BrowserShell)
  void SetOnSavePromptRequested(
      base::RepeatingCallback<void(const std::string& origin,
                                   const std::string& username,
                                   const std::string& password)> cb);

 private:
  void InjectFormDetector();
  void OnFormDetected(const std::string& result_json);
  void OnFormSubmitted(const std::string& result_json);

  content::WebContents* web_contents_;
  IAutofillStore* store_;
  IThemeProvider* theme_;

  base::RepeatingCallback<void(const std::string& origin,
                               const std::string& username,
                               const std::string& password)> on_save_prompt_;

  bool detector_injected_ = false;
  bool has_login_form_ = false;
  std::string current_origin_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
