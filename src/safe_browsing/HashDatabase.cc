// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "safe_browsing/HashDatabase.h"
#include "base/files/file_path.h"

#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "core/logging/VeorLogger.h"
#include "crypto/sha2.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

namespace veor {

namespace {

constexpr int kHashDbSchemaVersion = 1;

// Canonical URL for hashing: scheme + host + path (no query, no fragment).
std::string CanonicalUrlForHash(const GURL& url) {
  if (!url.is_valid())
    return std::string();
  std::string canonical = base::ToLowerASCII(url.scheme()) + "://" +
                          base::ToLowerASCII(url.host());
  if (url.has_port())
    canonical += ":" + url.port();
  canonical += url.path();
  return canonical;
}

}  // namespace

// ── HashDatabase ─────────────────────────────────────────────────────────────

HashDatabase::HashDatabase(const base::FilePath& db_path)
    : db_path_(db_path) {}

HashDatabase::~HashDatabase() {
  Close();
}

bool HashDatabase::Open() {
  db_ = std::make_unique<sql::Database>(sql::DatabaseOptions());
  if (!db_->Open(db_path_)) {
    VEOR_LOGE(LogCategory::kSecurity,
              "HashDatabase: failed to open " + db_path_.AsUTF8Unsafe());
    db_.reset();
    return false;
  }
  if (!EnsureSchema()) {
    db_->Close();
    db_.reset();
    return false;
  }
  return true;
}

void HashDatabase::Close() {
  if (db_) {
    db_->Close();
    db_.reset();
  }
}

bool HashDatabase::IsOpen() const {
  return db_ && db_->is_open();
}

bool HashDatabase::EnsureSchema() {
  if (db_->GetSchemaVersion() < kHashDbSchemaVersion) {
    sql::Transaction transaction(db_.get());
    if (!transaction.Begin())
      return false;

    if (!db_->Execute(
            "CREATE TABLE IF NOT EXISTS threat_hashes ("
            "  prefix BLOB PRIMARY KEY,"
            "  threat_type INTEGER NOT NULL,"
            "  source TEXT NOT NULL,"
            "  added_time INTEGER NOT NULL"
            ")"))
      return false;

    if (!db_->Execute(
            "CREATE INDEX IF NOT EXISTS idx_threat_type ON threat_hashes(threat_type)"))
      return false;

    if (!db_->SetSchemaVersion(kHashDbSchemaVersion))
      return false;

    if (!transaction.Commit())
      return false;
  }
  return true;
}

bool HashDatabase::InsertPrefixes(const std::vector<HashEntry>& entries) {
  if (!db_ || entries.empty())
    return false;

  sql::Transaction transaction(db_.get());
  if (!transaction.Begin())
    return false;

  sql::Statement stmt(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT OR REPLACE INTO threat_hashes (prefix, threat_type, source, added_time) "
      "VALUES (?, ?, ?, ?)"));

  int64_t now = base::Time::Now().ToDeltaSinceWindowsEpoch().InSeconds();
  for (const auto& entry : entries) {
    stmt.Reset(true);
    stmt.BindBlob(0, entry.prefix);
    stmt.BindInt(1, static_cast<int>(entry.threat_type));
    stmt.BindString(2, entry.source);
    stmt.BindInt64(3, entry.added_time > 0 ? entry.added_time : now);
    if (!stmt.Run())
      return false;
  }

  return transaction.Commit();
}

bool HashDatabase::ContainsUrl(const GURL& url, HashThreatType* out_type) const {
  if (!db_)
    return false;

  std::string prefix = ComputePrefix(url);
  if (prefix.empty())
    return false;

  sql::Statement stmt(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT threat_type FROM threat_hashes WHERE prefix = ? LIMIT 1"));
  stmt.BindBlob(0, prefix);

  if (!stmt.Step())
    return false;

  *out_type = static_cast<HashThreatType>(stmt.ColumnInt(0));
  return true;
}

bool HashDatabase::PurgeOldEntries(base::TimeDelta max_age) {
  if (!db_)
    return false;

  int64_t cutoff = (base::Time::Now() - max_age)
                       .ToDeltaSinceWindowsEpoch()
                       .InSeconds();

  sql::Statement stmt(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "DELETE FROM threat_hashes WHERE added_time < ?"));
  stmt.BindInt64(0, cutoff);
  return stmt.Run();
}

bool HashDatabase::ClearAll() {
  if (!db_)
    return false;
  return db_->Execute("DELETE FROM threat_hashes");
}

int64_t HashDatabase::GetEntryCount() const {
  if (!db_)
    return 0;
  sql::Statement stmt(db_->GetCachedStatement(
      SQL_FROM_HERE, "SELECT COUNT(*) FROM threat_hashes"));
  if (!stmt.Step())
    return 0;
  return stmt.ColumnInt64(0);
}

// ── ComputePrefix ────────────────────────────────────────────────────────────

std::string HashDatabase::ComputePrefix(const GURL& url) {
  std::string canonical = CanonicalUrlForHash(url);
  if (canonical.empty())
    return std::string();

  uint8_t hash[crypto::kSHA256Length];
  crypto::SHA256HashString(canonical, hash);

  // Return first 4 bytes (32-bit prefix) as used by Safe Browsing API v4.
  return std::string(reinterpret_cast<const char*>(hash), 4);
}

}  // namespace veor
