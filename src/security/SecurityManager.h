// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unordered_map>
#include "url/gurl.h"

#include "security/ISecurityManager.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SecurityManager
// ─────────────────────────────────────────────────────────────────────────────
// Validates certificates, manages permissions, enforces HTTPS.
// ─────────────────────────────────────────────────────────────────────────────

class SecurityManager : public ISecurityManager {
 public:
  SecurityManager();
  ~SecurityManager() override = default;

  // ISecurityManager
  Result<CertificateInfo, std::string> ValidateCertificate(
      const GURL& url) override;

  PermissionDecision GetPermissionDecision(
      const GURL& origin, PermissionType type) override;
  Result<void, std::string> SetPermissionDecision(
      const GURL& origin, PermissionType type, PermissionDecision decision) override;
  Result<void, std::string> ResetPermission(
      const GURL& origin, PermissionType type) override;

  bool IsHttpsRequired(const GURL& url) const override;
  Result<void, std::string> UpgradeToHttps(GURL& url) override;

  bool RequiresSiteIsolation(const GURL& url) const override;

 private:
  std::string OriginKey(const GURL& origin) const;

  struct PermissionKey {
    std::string origin;
    PermissionType type;
    bool operator==(const PermissionKey& other) const {
      return origin == other.origin && type == other.type;
    }
  };

  struct PermissionKeyHash {
    size_t operator()(const PermissionKey& key) const {
      return std::hash<std::string>{}(key.origin) ^
             std::hash<int>{}(static_cast<int>(key.type));
    }
  };

  std::unordered_map<PermissionKey, PermissionDecision, PermissionKeyHash>
      permission_cache_;
};

}  // namespace veor
