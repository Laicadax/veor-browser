// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/time/time.h"

namespace veor {

class SQLiteDatabase;

// ─────────────────────────────────────────────────────────────────────────────
// ThreatEntry
// ─────────────────────────────────────────────────────────────────────────────

struct ThreatEntry {
  std::string hash_prefix;      // 4-byte SHA256 prefix (hex)
  std::string threat_type;      // MALWARE, SOCIAL_ENGINEERING, UNWANTED_SOFTWARE
  base::Time last_update;
  int64_t cache_duration_sec;
};

// ─────────────────────────────────────────────────────────────────────────────
// ThreatDatabase
// ─────────────────────────────────────────────────────────────────────────────
// Local cache of Safe Browsing threat hashes.
// Schema: threats(hash_prefix, threat_type, last_update, cache_duration)
// ─────────────────────────────────────────────────────────────────────────────

class ThreatDatabase {
 public:
  ThreatDatabase();
  ~ThreatDatabase();

  bool Open(const base::FilePath& path);

  // Insert or replace entries
  void AddThreats(const std::vector<ThreatEntry>& entries);

  // Check if hash prefix is in database
  bool HasThreat(const std::string& hash_prefix, std::string* threat_type);

  // Delete expired entries
  void PruneExpired();

  // Clear all
  void ClearAll();

 private:
  void EnsureSchema();

  std::unique_ptr<SQLiteDatabase> db_;
};

}  // namespace veor
