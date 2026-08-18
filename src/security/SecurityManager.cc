// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "security/SecurityManager.h"
#include "url/gurl.h"

namespace veor {

SecurityManager::SecurityManager() = default;

Result<CertificateInfo, std::string> SecurityManager::ValidateCertificate(
    const GURL& url) {
  CertificateInfo info;
  if (url.SchemeIs("https")) {
    info.is_valid = true;
    info.days_until_expiry = 365;
  }
  return Result<CertificateInfo, std::string>::Ok(info);
}

PermissionDecision SecurityManager::GetPermissionDecision(
    const GURL& origin, PermissionType type) {
  PermissionKey key{OriginKey(origin), type};
  auto it = permission_cache_.find(key);
  return it != permission_cache_.end() ? it->second : PermissionDecision::kPrompt;
}

Result<void, std::string> SecurityManager::SetPermissionDecision(
    const GURL& origin, PermissionType type, PermissionDecision decision) {
  permission_cache_[PermissionKey{OriginKey(origin), type}] = decision;
  return Result<void, std::string>::Ok();
}

Result<void, std::string> SecurityManager::ResetPermission(
    const GURL& origin, PermissionType type) {
  permission_cache_.erase(PermissionKey{OriginKey(origin), type});
  return Result<void, std::string>::Ok();
}

bool SecurityManager::IsHttpsRequired(const GURL& url) const {
  return url.SchemeIs("https");
}

Result<void, std::string> SecurityManager::UpgradeToHttps(GURL& url) {
  if (url.SchemeIs("http")) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr("https");
    url = url.ReplaceComponents(replacements);
  }
  return Result<void, std::string>::Ok();
}

bool SecurityManager::RequiresSiteIsolation(const GURL& url) const {
  return url.SchemeIs("https");
}

std::string SecurityManager::OriginKey(const GURL& origin) const {
  return origin.GetOrigin().spec();
}

}  // namespace veor
