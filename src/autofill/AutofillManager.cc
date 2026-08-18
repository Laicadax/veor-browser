// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "autofill/AutofillManager.h"
#include "url/gurl.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "core/logging/VeorLogger.h"
#include "autofill/IAutofillStore.h"
#include "ui/theme/IThemeProvider.h"

namespace veor {

namespace {

// Minimal form detector injected into every page.
// Detects login forms, listens for submission, stores results in global.
constexpr char16_t kFormDetectorScript[] =
    u"(function() {"
    "  if (window.__veor_autofill_detector) return;"
    "  window.__veor_autofill_detector = true;"
    "  window.__veor_autofill_forms = [];"
    "  window.__veor_autofill_submitted = null;"
    "  function detectForms() {"
    "    var forms = document.querySelectorAll('form');"
    "    var result = [];"
    "    for (var i = 0; i < forms.length; i++) {"
    "      var f = forms[i];"
    "      var inputs = f.querySelectorAll('input');"
    "      var hasPassword = false;"
    "      var usernameField = null;"
    "      var passwordField = null;"
    "      for (var j = 0; j < inputs.length; j++) {"
    "        var inp = inputs[j];"
    "        var type = (inp.type || '').toLowerCase();"
    "        var name = (inp.name || '').toLowerCase();"
    "        if (type === 'password') {"
    "          hasPassword = true;"
    "          passwordField = inp;"
    "        } else if (type === 'text' || type === 'email') {"
    "          if (name.indexOf('user') !== -1 || name.indexOf('email') !== -1 ||"
    "              name.indexOf('login') !== -1) {"
    "            usernameField = inp;"
    "          }"
    "        }"
    "      }"
    "      if (hasPassword) {"
    "        result.push({"
    "          index: i,"
    "          hasUsername: !!usernameField,"
    "          usernameName: usernameField ? usernameField.name : ''"
    "        });"
    "        f.addEventListener('submit', function(e) {"
    "          var u = usernameField ? usernameField.value : '';"
    "          var p = passwordField ? passwordField.value : '';"
    "          if (p) {"
    "            window.__veor_autofill_submitted = {"
    "              username: u,"
    "              password: p,"
    "              origin: location.origin,"
    "              action: f.action || location.href"
    "            };"
    "          }"
    "        });"
    "      }"
    "    }"
    "    window.__veor_autofill_forms = result;"
    "    return result.length;"
    "  }"
    "  detectForms();"
    "  var observer = new MutationObserver(function(mutations) {"
    "    detectForms();"
    "  });"
    "  observer.observe(document.body, { childList: true, subtree: true });"
    "  return window.__veor_autofill_forms.length;"
    "})();";

// Script to read submitted credentials from global.
constexpr char16_t kReadSubmittedScript[] =
    u"(function() {"
    "  var s = window.__veor_autofill_submitted;"
    "  if (!s) return null;"
    "  window.__veor_autofill_submitted = null;"
    "  return JSON.stringify(s);"
    "})();";

// Script to fill credentials into detected form.
constexpr char kFillScript[] =
    "(function(username, password) {"
    "  var forms = document.querySelectorAll('form');"
    "  for (var i = 0; i < forms.length; i++) {"
    "    var f = forms[i];"
    "    var inputs = f.querySelectorAll('input');"
    "    var userField = null;"
    "    var passField = null;"
    "    for (var j = 0; j < inputs.length; j++) {"
    "      var inp = inputs[j];"
    "      var type = (inp.type || '').toLowerCase();"
    "      var name = (inp.name || '').toLowerCase();"
    "      if (type === 'password') passField = inp;"
    "      else if (type === 'text' || type === 'email') userField = inp;"
    "    }"
    "    if (passField) {"
    "      if (userField) userField.value = username;"
    "      passField.value = password;"
    "      return true;"
    "    }"
    "  }"
    "  return false;"
    "})('%s', '%s');";

std::string EscapeJsString(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '\\' || c == '\'' || c == '"')
      out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

}  // namespace

AutofillManager::AutofillManager(content::WebContents* web_contents,
                                 IAutofillStore* store,
                                 IThemeProvider* theme)
    : content::WebContentsObserver(web_contents),
      web_contents_(web_contents),
      store_(store),
      theme_(theme) {
  DCHECK(web_contents_);
  DCHECK(store_);
}

AutofillManager::~AutofillManager() = default;

void AutofillManager::SetOnSavePromptRequested(
    base::RepeatingCallback<void(const std::string& origin,
                                 const std::string& username,
                                 const std::string& password)> cb) {
  on_save_prompt_ = std::move(cb);
}

void AutofillManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() || navigation_handle->IsErrorPage())
    return;

  current_origin_ = navigation_handle->GetURL().GetOrigin().spec();
  detector_injected_ = false;
  has_login_form_ = false;
}

void AutofillManager::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                    const GURL& validated_url) {
  if (!render_frame_host->IsInPrimaryMainFrame())
    return;

  InjectFormDetector();
}

void AutofillManager::InjectFormDetector() {
  if (!web_contents_ || detector_injected_)
    return;

  auto* frame = web_contents_->GetPrimaryMainFrame();
  if (!frame)
    return;

  frame->ExecuteJavaScript(
      kFormDetectorScript,
      base::BindOnce([](base::Value result) {
        // Detector injected. Form count in result.
      }));

  detector_injected_ = true;

  // Check for existing credentials and fill after a short delay
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AutofillManager::CheckAndFillForms,
                     base::Unretained(this)),
      base::Milliseconds(800));
}

void AutofillManager::CheckAndFillForms() {
  if (!web_contents_ || current_origin_.empty())
    return;

  // Check if we have passwords for this origin
  auto result = store_->GetPasswordsForOrigin(current_origin_);
  if (result.IsErr() || result.Unwrap().empty())
    return;

  const auto& entries = result.Unwrap();
  // Fill the most recently used credential
  const auto& entry = entries[0];

  FillCredentials(entry.username, entry.password_encrypted);

  VEOR_LOGI(LogCategory::kSecurity,
            "Autofill filled credentials for " + current_origin_);
}

void AutofillManager::FillCredentials(const std::string& username,
                                      const std::string& password) {
  if (!web_contents_)
    return;

  auto* frame = web_contents_->GetPrimaryMainFrame();
  if (!frame)
    return;

  std::string script = base::StringPrintf(
      kFillScript,
      EscapeJsString(username).c_str(),
      EscapeJsString(password).c_str());

  frame->ExecuteJavaScript(
      base::UTF8ToUTF16(script),
      base::BindOnce([](base::Value result) {
        // Filled or not
      }));
}

void AutofillManager::PromptSavePassword(const std::string& origin,
                                         const std::string& username,
                                         const std::string& password) {
  if (on_save_prompt_) {
    on_save_prompt_.Run(origin, username, password);
  }
}

void AutofillManager::OnFormSubmitted(const std::string& result_json) {
  auto parsed = base::JSONReader::Read(result_json);
  if (!parsed || !parsed->is_dict())
    return;

  const auto& dict = parsed->GetDict();
  const std::string* username = dict.FindString("username");
  const std::string* password = dict.FindString("password");
  const std::string* origin = dict.FindString("origin");

  if (!username || !password || !origin)
    return;
  if (password->empty())
    return;

  // Check if we already have this credential
  auto existing = store_->GetPasswordsForOrigin(*origin);
  if (existing.IsOk()) {
    for (const auto& entry : existing.Unwrap()) {
      if (entry.username == *username && entry.password_encrypted == *password)
        return;  // Already saved
    }
  }

  PromptSavePassword(*origin, *username, *password);
}

}  // namespace veor
