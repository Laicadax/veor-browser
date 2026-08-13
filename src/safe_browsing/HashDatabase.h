// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "url/gurl.h"

namespace sql {
class Database;
}  // namespace sql

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// HashDatabase
// ─────────────────────────────────────────────────────────────────────────────
// Local SQLite-backed store of SHA256 hash prefixes for malicious URLs.
// Google Safe Browsing API v4 uses 4-byte (32-bit) prefixes.
//
// Schema:
//   threat_hashes(prefix BLOB PRIMARY KEY, threat_type INTEGER,
//                 source TEXT, added_time INTEGER)
//
// The prefix is the first 4 bytes of SHA256(full_url).
// ─────────────────────────────────────────────────────────────────────────────

enum class HashThreatType : int {
  kMalware = 1,
  kSocialEngineering = 2,
  kUnwantedSoftware = 3,
  kPotentiallyHarmful = 4,
};

struct HashEntry {
  std::string prefix;           // 4-byte SHA256 prefix
  HashThreatType threat_type;
  std::string source;         // e.g. "google_api", "local_list"
  int64_t added_time = 0;
};

class HashDatabase {
 public:
  explicit HashDatabase(const base::FilePath& db_path);
  ~HashDatabase();

  HashDatabase(const HashDatabase&) = delete;
  HashDatabase& operator=(const HashDatabase&) = delete;

  // Open or create database. Returns false on failure.
  bool Open();
  void Close();
  bool IsOpen() const;

  // Insert a batch of hash prefixes (atomic transaction).
  bool InsertPrefixes(const std::vector<HashEntry>& entries);

  // Check if a URL's hash prefix exists in the database.
  bool ContainsUrl(const GURL& url, HashThreatType* out_type) const;

  // Remove all entries older than max_age.
  bool PurgeOldEntries(base::TimeDelta max_age);

  // Clear entire database.
  bool ClearAll();

  // Statistics
  int64_t GetEntryCount() const;

  // Compute 4-byte SHA256 prefix for a URL.
  static std::string ComputePrefix(const GURL& url);

 private:
  bool EnsureSchema();

  base::FilePath db_path_;
  std::unique_ptr<sql::Database> db_;
};

}  // namespace veor