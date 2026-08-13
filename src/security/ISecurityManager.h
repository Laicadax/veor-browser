// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// PermissionType
// ─────────────────────────────────────────────────────────────────────────────

enum class PermissionType {
  kNotifications,
  kGeolocation,
  kCamera,
  kMicrophone,
  kClipboardRead,
  kClipboardWrite,
  kStorage,
  kJavaScript,
  kPopup,
};

// ─────────────────────────────────────────────────────────────────────────────
// PermissionDecision
// ─────────────────────────────────────────────────────────────────────────────

enum class PermissionDecision {
  kAllow,
  kDeny,
  kPrompt,
};

// ─────────────────────────────────────────────────────────────────────────────
// CertificateInfo
// ─────────────────────────────────────────────────────────────────────────────

struct CertificateInfo {
  bool is_valid = false;
  bool is_ev = false;
  std::string issuer;
  std::string subject;
  std::string fingerprint;
  int days_until_expiry = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ISecurityManager
// ─────────────────────────────────────────────────────────────────────────────

class ISecurityManager {
 public:
  virtual ~ISecurityManager() = default;

  // Certificate validation
  virtual Result<CertificateInfo, std::string> ValidateCertificate(
      const GURL& url) = 0;

  // Permission management
  virtual PermissionDecision GetPermissionDecision(
      const GURL& origin, PermissionType type) = 0;
  virtual Result<void, std::string> SetPermissionDecision(
      const GURL& origin, PermissionType type, PermissionDecision decision) = 0;
  virtual Result<void, std::string> ResetPermission(
      const GURL& origin, PermissionType type) = 0;

  // HTTPS enforcement
  virtual bool IsHttpsRequired(const GURL& url) const = 0;
  virtual Result<void, std::string> UpgradeToHttps(GURL& url) = 0;

  // Site isolation check
  virtual bool RequiresSiteIsolation(const GURL& url) const = 0;
};

}  // namespace veor
