// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// StorageError
// ─────────────────────────────────────────────────────────────────────────────

struct StorageError {
  int sqlite_code = 0;
  std::string message;
  std::string sql;  // The SQL that caused the error (if applicable)

  std::string ToString() const {
    return "SQLite[" + std::to_string(sqlite_code) + "]: " + message +
           (sql.empty() ? "" : " | SQL: " + sql);
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

class IStatement;
class ITransaction;

// ─────────────────────────────────────────────────────────────────────────────
// IStorageEngine
// ─────────────────────────────────────────────────────────────────────────────
// SQLite database wrapper with WAL mode, schema migrations, and prepared
// statements.
//
// Thread safety: [IO Thread] for all mutating operations.
//                Reads may be [Any Thread] if WAL mode is enabled and no
//                transaction is active.

class IStorageEngine {
 public:
  virtual ~IStorageEngine() = default;

  // ── Lifecycle ──

  // Opens or creates a database at the given path.
  // [IO Thread]
  virtual Result<void, StorageError> Open(const base::FilePath& path) = 0;

  // Closes the database. All pending writes are flushed.
  // [IO Thread]
  virtual Result<void, StorageError> Close() = 0;

  // Returns true if the database is open.
  virtual bool IsOpen() const = 0;

  // ── Schema ──

  // Returns the current schema version.
  virtual int GetVersion() const = 0;

  // Migrates the schema to the target version.
  // Migrations are applied transactionally.
  // [IO Thread]
  virtual Result<void, StorageError> Migrate(int target_version) = 0;

  // ── Raw Execution ──

  // Executes a SQL statement that does not return rows.
  // [IO Thread]
  virtual Result<void, StorageError> Execute(const std::string& sql) = 0;

  // Executes a SQL statement with bound parameters.
  // [IO Thread]
  virtual Result<void, StorageError> Execute(
      const std::string& sql,
      const std::vector<std::string>& params) = 0;

  // ── Prepared Statements ──

  // Prepares a statement for repeated execution.
  // [IO Thread]
  virtual Result<std::unique_ptr<IStatement>, StorageError> Prepare(
      const std::string& sql) = 0;

  // ── Transactions ──

  // Begins a transaction. RAII wrapper recommended.
  // [IO Thread]
  virtual Result<std::unique_ptr<ITransaction>, StorageError> BeginTransaction() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IStatement
// ─────────────────────────────────────────────────────────────────────────────

class IStatement {
 public:
  virtual ~IStatement() = default;

  virtual void BindInt(int index, int value) = 0;
  virtual void BindInt64(int index, int64_t value) = 0;
  virtual void BindDouble(int index, double value) = 0;
  virtual void BindString(int index, const std::string& value) = 0;
  virtual void BindBlob(int index, const std::vector<uint8_t>& value) = 0;
  virtual void BindNull(int index) = 0;

  // Executes the statement. Returns true if a row is available.
  // [IO Thread]
  virtual Result<bool, StorageError> Step() = 0;

  // Resets the statement for re-execution.
  virtual void Reset() = 0;

  // ── Column Access ──

  virtual int GetColumnCount() const = 0;
  virtual int GetInt(int column) const = 0;
  virtual int64_t GetInt64(int column) const = 0;
  virtual double GetDouble(int column) const = 0;
  virtual std::string GetString(int column) const = 0;
  virtual std::vector<uint8_t> GetBlob(int column) const = 0;
  virtual bool IsNull(int column) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ITransaction
// ─────────────────────────────────────────────────────────────────────────────
// RAII transaction wrapper. Auto-rollback in destructor if not committed.

class ITransaction {
 public:
  virtual ~ITransaction() = default;

  virtual Result<void, StorageError> Commit() = 0;
  virtual Result<void, StorageError> Rollback() = 0;
};

}  // namespace veor
