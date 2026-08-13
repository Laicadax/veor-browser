// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "url/gurl.h"

namespace veor {

class ThreatDatabase;

// ─────────────────────────────────────────────────────────────────────────────
// UrlChecker
// ─────────────────────────────────────────────────────────────────────────────
// Checks URLs against the local threat database using SHA256 prefix matching.
//
// Algorithm (Google Safe Browsing v4):
//   1. Canonicalize URL
//   2. Generate variations (host + path permutations)
//   3. SHA256 each variation
//   4. Check 4-byte prefix against local database
//   5. If prefix matches, mark as potentially malicious (full hash check deferred)
// ─────────────────────────────────────────────────────────────────────────────

class UrlChecker {
 public:
  explicit UrlChecker(ThreatDatabase* threat_db);
  ~UrlChecker();

  // Returns true if URL is known malicious (local prefix match)
  bool IsMalicious(const GURL& url, std::string* threat_type_out);

  // Generate canonical URL variations for hashing
  static std::vector<std::string> GenerateUrlVariations(const GURL& url);

 private:
  std::string ComputeHashPrefix(const std::string& input);

  ThreatDatabase* threat_db_;
};

}  // namespace veor
