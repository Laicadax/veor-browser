// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base/memory/raw_ptr.h"
#include "base/files/file_path.h"

#include "base/memory/raw_ptr.h"

#include "infrastructure/storage/IStorageEngine.h"

struct sqlite3;
struct sqlite3_stmt;

namespace veor {

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// StorageEngineImpl
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// SQLite-backed implementation of IStorageEngine.
//
// Features:
//   - WAL mode for concurrent reads/writes.
//   - Foreign keys enabled.
//   - Strict typing (SQLite 3.37+).
//   - Schema version tracking in user_version pragma.

class StorageEngineImpl : public IStorageEngine {
 public:
  StorageEngineImpl();
  ~StorageEngineImpl() override;

  // IStorageEngine
  Result<void, StorageError> Open(const base::FilePath& path) override;
  Result<void, StorageError> Close() override;
  bool IsOpen() const override;
  int GetVersion() const override;
  Result<void, StorageError> Migrate(int target_version) override;
  Result<void, StorageError> Execute(const std::string& sql) override;
  Result<void, StorageError> Execute(
      const std::string& sql,
      const std::vector<std::string>& params) override;
  Result<std::unique_ptr<IStatement>, StorageError> Prepare(
      const std::string& sql) override;
  Result<std::unique_ptr<ITransaction>, StorageError> BeginTransaction() override;

 private:
  raw_ptr<sqlite3> db_ = nullptr;
  bool is_open_ = false;
};

}  // namespace veor
