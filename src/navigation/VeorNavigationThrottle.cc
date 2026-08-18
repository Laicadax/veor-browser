// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "navigation/VeorNavigationThrottle.h"
#include "url/gurl.h"

#include "content/public/browser/navigation_handle.h"
#include "core/logging/VeorLogger.h"
#include "safe_browsing/SafeBrowsingService.h"

namespace veor {

VeorNavigationThrottle::VeorNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    SafeBrowsingService* safe_browsing)
    : content::NavigationThrottle(navigation_handle),
      safe_browsing_(safe_browsing) {}

VeorNavigationThrottle::~VeorNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
VeorNavigationThrottle::WillStartRequest() {
  return CheckUrl(navigation_handle()->GetURL());
}

content::NavigationThrottle::ThrottleCheckResult
VeorNavigationThrottle::WillRedirectRequest() {
  return CheckUrl(navigation_handle()->GetURL());
}

content::NavigationThrottle::ThrottleCheckResult
VeorNavigationThrottle::WillProcessResponse() {
  // Check SSL certificate validity for HTTPS navigations
  if (navigation_handle()->GetURL().SchemeIsCryptographic()) {
    net::SSLInfo ssl_info;
    if (navigation_handle()->GetSSLInfo(&ssl_info)) {
      return CheckSSL(ssl_info, navigation_handle()->GetURL());
    }
  }
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
VeorNavigationThrottle::WillFailRequest() {
  // Check if failure is due to SSL error
  if (navigation_handle()->GetNetErrorCode() == net::ERR_CERT_DATE_INVALID ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERT_AUTHORITY_INVALID ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERT_COMMON_NAME_INVALID ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERT_WEAK_SIGNATURE_ALGORITHM ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERT_NAME_CONSTRAINT_VIOLATION ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERT_VALIDITY_TOO_LONG ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERTIFICATE_TRANSPARENCY_REQUIRED ||
      navigation_handle()->GetNetErrorCode() == net::ERR_CERT_SYMANTEC_LEGACY ||
      navigation_handle()->GetNetErrorCode() == net::ERR_SSL_OBSOLETE_VERSION ||
      navigation_handle()->GetNetErrorCode() == net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN ||
      navigation_handle()->GetNetErrorCode() == net::ERR_SSL_PROTOCOL_ERROR) {
    VEOR_LOGW(LogCategory::kSecurity,
              "SSL error for " + navigation_handle()->GetURL().host() +
                  ": " + std::to_string(navigation_handle()->GetNetErrorCode()));
    return content::NavigationThrottle::CANCEL;
  }
  return content::NavigationThrottle::PROCEED;
}

const char* VeorNavigationThrottle::GetNameForLogging() {
  return "VeorNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
VeorNavigationThrottle::CheckUrl(const GURL& url) {
  if (!safe_browsing_)
    return content::NavigationThrottle::PROCEED;

  auto result = safe_browsing_->CheckUrlSync(url);
  if (result.threat_type == ThreatType::kSafe)
    return content::NavigationThrottle::PROCEED;

  VEOR_LOGW(LogCategory::kSecurity,
            "NavigationThrottle blocked: " + url.spec() +
                " [threat=" + std::to_string(static_cast<int>(result.threat_type)) + "]");

  return content::NavigationThrottle::CANCEL;
}

content::NavigationThrottle::ThrottleCheckResult
VeorNavigationThrottle::CheckSSL(const net::SSLInfo& ssl_info,
                                 const GURL& url) {
  if (ssl_info.cert_status & net::CERT_STATUS_ALL_ERRORS) {
    int cert_status = ssl_info.cert_status;
    std::string error_detail;

    if (cert_status & net::CERT_STATUS_COMMON_NAME_INVALID)
      error_detail = "COMMON_NAME_INVALID";
    else if (cert_status & net::CERT_STATUS_DATE_INVALID)
      error_detail = "DATE_INVALID";
    else if (cert_status & net::CERT_STATUS_AUTHORITY_INVALID)
      error_detail = "AUTHORITY_INVALID";
    else if (cert_status & net::CERT_STATUS_WEAK_SIGNATURE_ALGORITHM)
      error_detail = "WEAK_SIGNATURE";
    else if (cert_status & net::CERT_STATUS_NO_REVOCATION_MECHANISM)
      error_detail = "NO_REVOCATION";
    else if (cert_status & net::CERT_STATUS_UNABLE_TO_CHECK_REVOCATION)
      error_detail = "UNABLE_TO_CHECK_REVOCATION";
    else if (cert_status & net::CERT_STATUS_REVOKED)
      error_detail = "REVOKED";
    else if (cert_status & net::CERT_STATUS_INVALID)
      error_detail = "INVALID";
    else if (cert_status & net::CERT_STATUS_PINNED_KEY_MISSING)
      error_detail = "PINNED_KEY_MISSING";
    else if (cert_status & net::CERT_STATUS_NAME_CONSTRAINT_VIOLATION)
      error_detail = "NAME_CONSTRAINT_VIOLATION";
    else if (cert_status & net::CERT_STATUS_VALIDITY_TOO_LONG)
      error_detail = "VALIDITY_TOO_LONG";
    else if (cert_status & net::CERT_STATUS_CERTIFICATE_TRANSPARENCY_REQUIRED)
      error_detail = "TRANSPARENCY_REQUIRED";
    else if (cert_status & net::CERT_STATUS_SYMANTEC_LEGACY)
      error_detail = "SYMANTEC_LEGACY";
    else
      error_detail = "UNKNOWN_SSL_ERROR";

    VEOR_LOGW(LogCategory::kSecurity,
              "SSL certificate error for " + url.host() +
                  ": " + error_detail + " (status=" +
                  std::to_string(cert_status) + ")");

    // Check if user has explicitly bypassed this error before
    // TODO: Store SSL exception decisions in settings
    bool allow_bypass = false;
    if (!allow_bypass) {
      return content::NavigationThrottle::CANCEL;
    }
  }

  return content::NavigationThrottle::PROCEED;
}

}  // namespace veor
