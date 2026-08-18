// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/SQLiteDatabase.h"
#include "base/files/file_path.h"

#include "base/logging.h"
#include "core/logging/VeorLogger.h"

namespace veor {

SQLiteDatabase::SQLiteDatabase() = default;
SQLiteDatabase::~SQLiteDatabase() {
  if (in_transaction_)
    RollbackTransaction();
  Close();
}

bool SQLiteDatabase::Open(const base::FilePath& path) {
  db_ = std::make_unique<sql::Database>(sql::DatabaseOptions());
  if (!db_->Open(path)) {
    VEOR_LOGE(LogCategory::kStorage,
              "Failed to open database: " + path.AsUTF8Unsafe());
    db_.reset();
    return false;
  }
  VEOR_LOGI(LogCategory::kStorage,
            "Database opened: " + path.AsUTF8Unsafe());
  return true;
}

void SQLiteDatabase::Close() {
  if (db_) {
    db_->Close();
    db_.reset();
  }
}

bool SQLiteDatabase::IsOpen() const {
  return db_ && db_->is_open();
}

bool SQLiteDatabase::Execute(const std::string& sql) {
  if (!db_)
    return false;
  return db_->Execute(sql.c_str());
}

sql::Statement SQLiteDatabase::Prepare(const std::string& sql) {
  if (!db_)
    return sql::Statement();
  return sql::Statement(db_->GetUniqueStatement(sql.c_str()));
}

bool SQLiteDatabase::BeginTransaction() {
  if (!db_ || in_transaction_)
    return false;
  in_transaction_ = db_->BeginTransaction();
  return in_transaction_;
}

bool SQLiteDatabase::CommitTransaction() {
  if (!db_ || !in_transaction_)
    return false;
  in_transaction_ = false;
  return db_->CommitTransaction();
}

void SQLiteDatabase::RollbackTransaction() {
  if (db_ && in_transaction_) {
    db_->RollbackTransaction();
    in_transaction_ = false;
  }
}

int SQLiteDatabase::GetSchemaVersion() {
  if (!db_)
    return 0;
  return db_->GetSchemaVersion();
}

bool SQLiteDatabase::SetSchemaVersion(int version) {
  if (!db_)
    return false;
  return db_->SetSchemaVersion(version);
}

// ── SQLTransaction ───────────────────────────────────────────────────────────

SQLTransaction::SQLTransaction(SQLiteDatabase* db) : db_(db) {}

SQLTransaction::~SQLTransaction() {
  if (!committed_ && db_)
    db_->RollbackTransaction();
}

bool SQLTransaction::Begin() {
  return db_ && db_->BeginTransaction();
}

bool SQLTransaction::Commit() {
  if (!db_)
    return false;
  committed_ = db_->CommitTransaction();
  return committed_;
}

}  // namespace veor
