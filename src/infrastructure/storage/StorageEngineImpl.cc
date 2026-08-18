// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "infrastructure/storage/StorageEngineImpl.h"

#include "third_party/sqlite/sqlite3.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "core/logging/VeorLogger.h"

namespace veor {

namespace {

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ SQLite error wrapper ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
StorageError MakeError(sqlite3* db, int rc, const std::string& sql = {}) {
  return StorageError{
      rc,
      sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "Unknown SQLite error",
      sql};
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ Statement implementation ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
class StatementImpl : public IStatement {
 public:
  StatementImpl(sqlite3_stmt* stmt, sqlite3* db)
      : stmt_(stmt), db_(db) {}

  ~StatementImpl() override {
    if (stmt_) {
      sqlite3_finalize(stmt_);
    }
  }

  void BindInt(int index, int value) override {
    sqlite3_bind_int(stmt_, index, value);
  }

  void BindInt64(int index, int64_t value) override {
    sqlite3_bind_int64(stmt_, index, value);
  }

  void BindDouble(int index, double value) override {
    sqlite3_bind_double(stmt_, index, value);
  }

  void BindString(int index, const std::string& value) override {
    sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
  }

  void BindBlob(int index, const std::vector<uint8_t>& value) override {
    sqlite3_bind_blob(stmt_, index, value.data(), static_cast<int>(value.size()),
                      SQLITE_TRANSIENT);
  }

  void BindNull(int index) override {
    sqlite3_bind_null(stmt_, index);
  }

  Result<bool, StorageError> Step() override {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
      return Result<bool, StorageError>::Ok(true);
    }
    if (rc == SQLITE_DONE) {
      return Result<bool, StorageError>::Ok(false);
    }
    return Result<bool, StorageError>::Err(MakeError(db_, rc));
  }

  void Reset() override {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
  }

  int GetColumnCount() const override {
    return sqlite3_column_count(stmt_);
  }

  int GetInt(int column) const override {
    return sqlite3_column_int(stmt_, column);
  }

  int64_t GetInt64(int column) const override {
    return sqlite3_column_int64(stmt_, column);
  }

  double GetDouble(int column) const override {
    return sqlite3_column_double(stmt_, column);
  }

  std::string GetString(int column) const override {
    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
    return text ? text : "";
  }

  std::vector<uint8_t> GetBlob(int column) const override {
    const void* blob = sqlite3_column_blob(stmt_, column);
    int size = sqlite3_column_bytes(stmt_, column);
    if (blob && size > 0) {
      const uint8_t* data = static_cast<const uint8_t*>(blob);
      return std::vector<uint8_t>(data, data + size);
    }
    return {};
  }

  bool IsNull(int column) const override {
    return sqlite3_column_type(stmt_, column) == SQLITE_NULL;
  }

 private:
  raw_ptr<sqlite3_stmt> stmt_;
  sqlite3* db_;
};

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ Transaction implementation ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
class TransactionImpl : public ITransaction {
 public:
  explicit TransactionImpl(sqlite3* db) : db_(db), committed_(false) {
    sqlite3_exec(db_, "BEGIN", nullptr, nullptr, nullptr);
  }

  ~TransactionImpl() override {
    if (!committed_) {
      sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
    }
  }

  Result<void, StorageError> Commit() override {
    int rc = sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
      return Result<void, StorageError>::Err(MakeError(db_, rc));
    }
    committed_ = true;
    return Result<void, StorageError>::Ok();
  }

  Result<void, StorageError> Rollback() override {
    int rc = sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
      return Result<void, StorageError>::Err(MakeError(db_, rc));
    }
    committed_ = true;  // Mark as handled to prevent double-rollback in dtor.
    return Result<void, StorageError>::Ok();
  }

 private:
  raw_ptr<sqlite3> db_;\r?n  bool committed_;
};

}  // namespace

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
// StorageEngineImpl
// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

StorageEngineImpl::StorageEngineImpl() = default;

StorageEngineImpl::~StorageEngineImpl() {
  if (is_open_) {
    Close();
  }
}

Result<void, StorageError> StorageEngineImpl::Open(const base::FilePath& path) {
  if (is_open_) {
    return Result<void, StorageError>::Err(
        MakeError(db_, SQLITE_MISUSE, "Database already open"));
  }

  int rc = sqlite3_open(base::WideToUTF8(path.Unwrap()).c_str(), &db_);
  if (rc != SQLITE_OK) {
    auto err = MakeError(db_, rc);
    sqlite3_close(db_);
    db_ = nullptr;
    return Result<void, StorageError>::Err(err);
  }

  // Enable WAL mode for concurrent reads/writes.
  rc = sqlite3_exec(db_, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    VEOR_LOGW(LogCategory::kInfrastructure,
              "Failed to enable WAL mode: " + std::string(sqlite3_errmsg(db_)));
  }

  // Enable foreign keys.
  sqlite3_exec(db_, "PRAGMA foreign_keys=ON", nullptr, nullptr, nullptr);

  // Enable strict typing (SQLite 3.37+).
  sqlite3_exec(db_, "PRAGMA strict=ON", nullptr, nullptr, nullptr);

  is_open_ = true;
  VEOR_LOGI(LogCategory::kInfrastructure,
            "StorageEngine opened: " + base::WideToUTF8(path.Unwrap()));
  return Result<void, StorageError>::Ok();
}

Result<void, StorageError> StorageEngineImpl::Close() {
  if (!is_open_) {
    return Result<void, StorageError>::Ok();
  }

  int rc = sqlite3_close(db_);
  if (rc != SQLITE_OK) {
    return Result<void, StorageError>::Err(MakeError(db_, rc));
  }

  db_ = nullptr;
  is_open_ = false;
  VEOR_LOGI(LogCategory::kInfrastructure, "StorageEngine closed");
  return Result<void, StorageError>::Ok();
}

bool StorageEngineImpl::IsOpen() const {
  return is_open_;
}

int StorageEngineImpl::GetVersion() const {
  if (!is_open_)
    return 0;

  sqlite3_stmt* stmt = nullptr;
  sqlite3_prepare_v2(db_, "PRAGMA user_version", -1, &stmt, nullptr);
  int version = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    version = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return version;
}

Result<void, StorageError> StorageEngineImpl::Migrate(int target_version) {
  if (!is_open_) {
    return Result<void, StorageError>::Err(
        MakeError(db_, SQLITE_MISUSE, "Database not open"));
  }

  int current = GetVersion();
  if (current == target_version) {
    return Result<void, StorageError>::Ok();
  }

  if (current > target_version) {
    return Result<void, StorageError>::Err(
        MakeError(db_, SQLITE_MISUSE,
                  "Cannot downgrade from version " + std::to_string(current) +
                      " to " + std::to_string(target_version)));
  }

  // Migrations are applied by the caller via Execute() calls.
  // This method just validates and sets the version.
  std::string sql = "PRAGMA user_version = " + std::to_string(target_version);
  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    return Result<void, StorageError>::Err(MakeError(db_, rc, sql));
  }

  VEOR_LOGI(LogCategory::kInfrastructure,
            "Schema migrated: " + std::to_string(current) + " -> " +
                std::to_string(target_version));
  return Result<void, StorageError>::Ok();
}

Result<void, StorageError> StorageEngineImpl::Execute(const std::string& sql) {
  if (!is_open_) {
    return Result<void, StorageError>::Err(
        MakeError(db_, SQLITE_MISUSE, "Database not open"));
  }

  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    return Result<void, StorageError>::Err(MakeError(db_, rc, sql));
  }
  return Result<void, StorageError>::Ok();
}

Result<void, StorageError> StorageEngineImpl::Execute(
    const std::string& sql,
    const std::vector<std::string>& params) {
  auto stmt_result = Prepare(sql);
  if (stmt_result.IsErr()) {
    return Result<void, StorageError>::Err(stmt_result.UnwrapErr());
  }

  auto stmt = std::move(stmt_result).Unwrap();
  for (size_t i = 0; i < params.size(); ++i) {
    stmt->BindString(static_cast<int>(i + 1), params[i]);
  }

  auto step_result = stmt->Step();
  if (step_result.IsErr()) {
    return Result<void, StorageError>::Err(step_result.UnwrapErr());
  }

  return Result<void, StorageError>::Ok();
}

Result<std::unique_ptr<IStatement>, StorageError> StorageEngineImpl::Prepare(
    const std::string& sql) {
  if (!is_open_) {
    return Result<std::unique_ptr<IStatement>, StorageError>::Err(
        MakeError(db_, SQLITE_MISUSE, "Database not open"));
  }

  sqlite3_stmt* raw_stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &raw_stmt, nullptr);
  if (rc != SQLITE_OK) {
    return Result<std::unique_ptr<IStatement>, StorageError>::Err(
        MakeError(db_, rc, sql));
  }

  return Result<std::unique_ptr<IStatement>, StorageError>::Ok(
      std::make_unique<StatementImpl>(raw_stmt, db_));
}

Result<std::unique_ptr<ITransaction>, StorageError> StorageEngineImpl::BeginTransaction() {
  if (!is_open_) {
    return Result<std::unique_ptr<ITransaction>, StorageError>::Err(
        MakeError(db_, SQLITE_MISUSE, "Database not open"));
  }

  return Result<std::unique_ptr<ITransaction>, StorageError>::Ok(
      std::make_unique<TransactionImpl>(db_));
}

}  // namespace veor
