// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "infrastructure/storage/StorageEngineImpl.h"
#include "core/test/VeorTestBase.h"
#include "base/files/scoped_temp_dir.h"

namespace veor {

class StorageEngineTest : public VeorTestBase {
 protected:
  void SetUp() override {
    VeorTestBase::SetUp();
    engine_ = std::make_unique<StorageEngineImpl>();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    db_path_ = temp_dir_.GetPath().AppendASCII("test.db");
  }

  std::unique_ptr<StorageEngineImpl> engine_;
  base::ScopedTempDir temp_dir_;
  base::FilePath db_path_;
};

TEST_F(StorageEngineTest, OpenClose) {
  EXPECT_FALSE(engine_->IsOpen());
  auto result = engine_->Open(db_path_);
  EXPECT_TRUE(result.IsOk());
  EXPECT_TRUE(engine_->IsOpen());

  auto close_result = engine_->Close();
  EXPECT_TRUE(close_result.IsOk());
  EXPECT_FALSE(engine_->IsOpen());
}

TEST_F(StorageEngineTest, ExecuteCreateTable) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());

  auto result = engine_->Execute(
      "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
  EXPECT_TRUE(result.IsOk());
}

TEST_F(StorageEngineTest, ExecuteInsertAndSelect) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());
  ASSERT_TRUE(engine_->Execute(
      "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)").IsOk());

  auto insert = engine_->Execute(
      "INSERT INTO test (name) VALUES ('hello')");
  EXPECT_TRUE(insert.IsOk());

  auto stmt_result = engine_->Prepare("SELECT name FROM test WHERE id = 1");
  ASSERT_TRUE(stmt_result.IsOk());

  auto stmt = std::move(stmt_result).Unwrap();
  auto step = stmt->Step();
  ASSERT_TRUE(step.IsOk());
  EXPECT_TRUE(step.Unwrap());

  EXPECT_EQ(stmt->GetString(0), "hello");
}

TEST_F(StorageEngineTest, PreparedStatementBinding) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());
  ASSERT_TRUE(engine_->Execute(
      "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value REAL)").IsOk());

  auto stmt_result = engine_->Prepare(
      "INSERT INTO test (name, value) VALUES (?, ?)");
  ASSERT_TRUE(stmt_result.IsOk());

  auto stmt = std::move(stmt_result).Unwrap();
  stmt->BindString(1, "test");
  stmt->BindDouble(2, 3.14);

  auto step = stmt->Step();
  EXPECT_TRUE(step.IsOk());
  EXPECT_FALSE(step.Unwrap());  // INSERT returns no rows

  // Verify
  auto select = engine_->Prepare("SELECT name, value FROM test");
  ASSERT_TRUE(select.IsOk());
  auto s = std::move(select).Unwrap();
  ASSERT_TRUE(s->Step().Unwrap());
  EXPECT_EQ(s->GetString(0), "test");
  EXPECT_DOUBLE_EQ(s->GetDouble(1), 3.14);
}

TEST_F(StorageEngineTest, TransactionCommit) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());
  ASSERT_TRUE(engine_->Execute(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)").IsOk());

  {
    auto tx_result = engine_->BeginTransaction();
    ASSERT_TRUE(tx_result.IsOk());
    auto tx = std::move(tx_result).Unwrap();

    EXPECT_TRUE(engine_->Execute("INSERT INTO test DEFAULT VALUES").IsOk());
    EXPECT_TRUE(tx->Commit().IsOk());
  }

  auto select = engine_->Prepare("SELECT COUNT(*) FROM test");
  ASSERT_TRUE(select.IsOk());
  auto s = std::move(select).Unwrap();
  ASSERT_TRUE(s->Step().Unwrap());
  EXPECT_EQ(s->GetInt(0), 1);
}

TEST_F(StorageEngineTest, TransactionRollback) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());
  ASSERT_TRUE(engine_->Execute(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)").IsOk());

  {
    auto tx_result = engine_->BeginTransaction();
    ASSERT_TRUE(tx_result.IsOk());
    auto tx = std::move(tx_result).Unwrap();

    EXPECT_TRUE(engine_->Execute("INSERT INTO test DEFAULT VALUES").IsOk());
    // tx goes out of scope without Commit -> auto-rollback
  }

  auto select = engine_->Prepare("SELECT COUNT(*) FROM test");
  ASSERT_TRUE(select.IsOk());
  auto s = std::move(select).Unwrap();
  ASSERT_TRUE(s->Step().Unwrap());
  EXPECT_EQ(s->GetInt(0), 0);
}

TEST_F(StorageEngineTest, SchemaVersion) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());
  EXPECT_EQ(engine_->GetVersion(), 0);

  auto migrate = engine_->Migrate(1);
  EXPECT_TRUE(migrate.IsOk());
  EXPECT_EQ(engine_->GetVersion(), 1);
}

TEST_F(StorageEngineTest, BlobStorage) {
  ASSERT_TRUE(engine_->Open(db_path_).IsOk());
  ASSERT_TRUE(engine_->Execute(
      "CREATE TABLE blobs (id INTEGER PRIMARY KEY, data BLOB)").IsOk());

  std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF};

  auto stmt_result = engine_->Prepare("INSERT INTO blobs (data) VALUES (?)");
  ASSERT_TRUE(stmt_result.IsOk());
  auto stmt = std::move(stmt_result).Unwrap();
  stmt->BindBlob(1, data);
  EXPECT_TRUE(stmt->Step().IsOk());

  auto select = engine_->Prepare("SELECT data FROM blobs");
  ASSERT_TRUE(select.IsOk());
  auto s = std::move(select).Unwrap();
  ASSERT_TRUE(s->Step().Unwrap());
  EXPECT_EQ(s->GetBlob(0), data);
}

}  // namespace veor
