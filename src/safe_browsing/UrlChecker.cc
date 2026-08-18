// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "safe_browsing/UrlChecker.h"
#include "url/gurl.h"

#include "base/strings/string_util.h"
#include "crypto/sha2.h"
#include "net/base/hash_value.h"
#include "safe_browsing/ThreatDatabase.h"

namespace veor {

UrlChecker::UrlChecker(ThreatDatabase* threat_db) : threat_db_(threat_db) {}

UrlChecker::~UrlChecker() = default;

bool UrlChecker::IsMalicious(const GURL& url, std::string* threat_type_out) {
  if (!threat_db_ || !url.is_valid())
    return false;

  auto variations = GenerateUrlVariations(url);
  for (const auto& variation : variations) {
    std::string prefix = ComputeHashPrefix(variation);
    if (threat_db_->HasThreat(prefix, threat_type_out))
      return true;
  }
  return false;
}

std::vector<std::string> UrlChecker::GenerateUrlVariations(const GURL& url) {
  std::vector<std::string> variations;

  // Canonical form: scheme://host/path
  std::string host = base::ToLowerASCII(url.host());
  std::string path = url.path();
  if (path.empty())
    path = "/";

  // Variation 1: host + path
  variations.push_back(host + path);

  // Variation 2: host only (for domain-level blocks)
  variations.push_back(host + "/");

  // Variation 3: host + path without query
  // (already done above, URL::path() excludes query)

  // Variation 4: www. prefix stripped/added
  if (base::StartsWith(host, "www.", base::CompareCase::SENSITIVE)) {
    variations.push_back(host.substr(4) + path);
  } else {
    variations.push_back("www." + host + path);
  }

  return variations;
}

std::string UrlChecker::ComputeHashPrefix(const std::string& input) {
  // SHA256, return first 4 bytes as hex string
  uint8_t hash[crypto::kSHA256Length];
  crypto::SHA256HashString(input, hash);

  // First 4 bytes as hex
  static const char kHex[] = "0123456789abcdef";
  std::string result;
  result.reserve(8);
  for (int i = 0; i < 4; ++i) {
    result += kHex[(hash[i] >> 4) & 0xF];
    result += kHex[hash[i] & 0xF];
  }
  return result;
}

}  // namespace veor
