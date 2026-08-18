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

class IStatement;
class ITransaction;

class IStorageEngine {
 public:
  virtual ~IStorageEngine() = default;
  virtual Result<void, StorageError> Open(const base::FilePath& path) = 0;
  virtual Result<void, StorageError> Close() = 0;
  virtual bool IsOpen() const = 0;
  virtual int GetVersion() const = 0;
  virtual Result<void, StorageError> Migrate(int target_version) = 0;
  virtual Result<void, StorageError> Execute(const std::string& sql) = 0;
  virtual Result<void, StorageError> Execute(const std::string& sql, const std::vector<std::string>& params) = 0;
  virtual Result<std::unique_ptr<IStatement>, StorageError> Prepare(const std::string& sql) = 0;
  virtual Result<std::unique_ptr<ITransaction>, StorageError> BeginTransaction() = 0;
};

class IStatement {
 public:
  virtual ~IStatement() = default;
  virtual void BindInt(int index, int value) = 0;
  virtual void BindInt64(int index, int64_t value) = 0;
  virtual void BindDouble(int index, double value) = 0;
  virtual void BindString(int index, const std::string& value) = 0;
  virtual void BindBlob(int index, const std::vector<uint8_t>& value) = 0;
  virtual void BindNull(int index) = 0;
  virtual Result<bool, StorageError> Step() = 0;
  virtual void Reset() = 0;
  virtual int GetColumnCount() const = 0;
  virtual int GetInt(int column) const = 0;
  virtual int64_t GetInt64(int column) const = 0;
  virtual double GetDouble(int column) const = 0;
  virtual std::string GetString(int column) const = 0;
  virtual std::vector<uint8_t> GetBlob(int column) const = 0;
  virtual bool IsNull(int column) const = 0;
};

class ITransaction {
 public:
  virtual ~ITransaction() = default;
  virtual Result<void, StorageError> Commit() = 0;
  virtual Result<void, StorageError> Rollback() = 0;
};

}  // namespace veor
