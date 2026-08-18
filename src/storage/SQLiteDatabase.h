// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "base/memory/raw_ptr.h"
#include <string>

#include "base/files/file_path.h"
#include "sql/database.h"
#include "sql/statement.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SQLiteDatabase
// ─────────────────────────────────────────────────────────────────────────────
// RAII wrapper around sql::Database. Provides:
//   - Open/Close lifecycle
//   - Transaction scope guard
//   - Schema version tracking (migrations)
// ─────────────────────────────────────────────────────────────────────────────

class SQLiteDatabase {
 public:
  SQLiteDatabase();
  ~SQLiteDatabase();

  // Open or create database at path. Returns false on failure.
  bool Open(const base::FilePath& path);
  void Close();
  bool IsOpen() const;

  // Execute raw SQL. Returns false on error.
  bool Execute(const std::string& sql);

  // Prepared statement helper
  sql::Statement Prepare(const std::string& sql);

  // Transactions
  bool BeginTransaction();
  bool CommitTransaction();
  void RollbackTransaction();

  // Schema version for migrations
  int GetSchemaVersion();
  bool SetSchemaVersion(int version);

  sql::Database* db() { return db_.get(); }

 private:
  std::unique_ptr<sql::Database> db_;
  bool in_transaction_ = false;
};

// RAII transaction guard
class SQLTransaction {
 public:
  explicit SQLTransaction(SQLiteDatabase* db);
  ~SQLTransaction();

  bool Begin();
  bool Commit();

 private:
  raw_ptr<SQLiteDatabase> db_;
  bool committed_ = false;
};

}  // namespace veor
