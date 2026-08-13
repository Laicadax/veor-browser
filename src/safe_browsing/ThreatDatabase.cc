// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "safe_browsing/ThreatDatabase.h"

#include "base/time/time.h"
#include "sql/statement.h"
#include "storage/SQLiteDatabase.h"

namespace veor {

namespace {

constexpr int kCurrentSchemaVersion = 1;

const char kCreateThreatsTable[] =
    "CREATE TABLE IF NOT EXISTS threats ("
    "  hash_prefix TEXT PRIMARY KEY,"
    "  threat_type TEXT NOT NULL,"
    "  last_update INTEGER NOT NULL,"
    "  cache_duration INTEGER NOT NULL"
    ")";

const char kCreateHashIndex[] =
    "CREATE INDEX IF NOT EXISTS idx_threats_hash ON threats(hash_prefix)";

}  // namespace

ThreatDatabase::ThreatDatabase() = default;
ThreatDatabase::~ThreatDatabase() = default;

bool ThreatDatabase::Open(const base::FilePath& path) {
  db_ = std::make_unique<SQLiteDatabase>();
  if (!db_->Open(path))
    return false;
  EnsureSchema();
  return true;
}

void ThreatDatabase::EnsureSchema() {
  if (db_->GetSchemaVersion() < kCurrentSchemaVersion) {
    db_->Execute(kCreateThreatsTable);
    db_->Execute(kCreateHashIndex);
    db_->SetSchemaVersion(kCurrentSchemaVersion);
  }
}

void ThreatDatabase::AddThreats(const std::vector<ThreatEntry>& entries) {
  if (!db_ || !db_->IsOpen())
    return;

  SQLTransaction txn(db_.get());
  if (!txn.Begin())
    return;

  sql::Statement insert = db_->Prepare(
      "INSERT OR REPLACE INTO threats "
      "(hash_prefix, threat_type, last_update, cache_duration) "
      "VALUES (?, ?, ?, ?)");

  for (const auto& entry : entries) {
    insert.Reset(true);
    insert.BindString(0, entry.hash_prefix);
    insert.BindString(1, entry.threat_type);
    insert.BindInt64(2, entry.last_update.ToInternalValue());
    insert.BindInt64(3, entry.cache_duration_sec);
    insert.Run();
  }

  txn.Commit();
}

bool ThreatDatabase::HasThreat(const std::string& hash_prefix,
                               std::string* threat_type) {
  if (!db_ || !db_->IsOpen())
    return false;

  sql::Statement query = db_->Prepare(
      "SELECT threat_type FROM threats WHERE hash_prefix = ? "
      "AND (last_update + cache_duration * 1000000) > ?");
  query.BindString(0, hash_prefix);
  query.BindInt64(1, base::Time::Now().ToInternalValue());

  if (query.Step()) {
    if (threat_type)
      *threat_type = query.ColumnString(0);
    return true;
  }
  return false;
}

void ThreatDatabase::PruneExpired() {
  if (!db_ || !db_->IsOpen())
    return;
  sql::Statement del = db_->Prepare(
      "DELETE FROM threats WHERE (last_update + cache_duration * 1000000) <= ?");
  del.BindInt64(0, base::Time::Now().ToInternalValue());
  del.Run();
}

void ThreatDatabase::ClearAll() {
  if (!db_ || !db_->IsOpen())
    return;
  db_->Execute("DELETE FROM threats");
}

}  // namespace veor
