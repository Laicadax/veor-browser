// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "core/config/ConfigProviderImpl.h"
#include "core/threading/TaskRunnerFactory.h"
#include "core/test/VeorTestBase.h"
#include "base/files/scoped_temp_dir.h"

namespace veor {

class ConfigProviderTest : public VeorTestBase {
 protected:
  void SetUp() override {
    VeorTestBase::SetUp();
    factory_ = std::make_unique<TaskRunnerFactory>();
    provider_ = std::make_unique<ConfigProviderImpl>(factory_->GetIoRunner());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    path_ = temp_dir_.GetPath().AppendASCII("config.json").AsUTF8Unsafe();
    provider_->SetFilePath(path_);
  }

  std::unique_ptr<TaskRunnerFactory> factory_;
  std::unique_ptr<ConfigProviderImpl> provider_;
  base::ScopedTempDir temp_dir_;
  std::string path_;
};

TEST_F(ConfigProviderTest, SetAndGetBool) {
  provider_->Set("test.bool", ConfigValue(true));
  EXPECT_TRUE(provider_->GetBool("test.bool", false));
}

TEST_F(ConfigProviderTest, SetAndGetInt) {
  provider_->Set("test.int", ConfigValue(42));
  EXPECT_EQ(provider_->GetInt("test.int", 0), 42);
}

TEST_F(ConfigProviderTest, SetAndGetDouble) {
  provider_->Set("test.double", ConfigValue(3.14));
  EXPECT_DOUBLE_EQ(provider_->GetDouble("test.double", 0.0), 3.14);
}

TEST_F(ConfigProviderTest, SetAndGetString) {
  provider_->Set("test.str", ConfigValue(std::string("hello")));
  EXPECT_EQ(provider_->GetString("test.str"), "hello");
}

TEST_F(ConfigProviderTest, DefaultValueWhenMissing) {
  EXPECT_EQ(provider_->GetInt("missing", 99), 99);
  EXPECT_EQ(provider_->GetBool("missing", true), true);
  EXPECT_DOUBLE_EQ(provider_->GetDouble("missing", 1.5), 1.5);
  EXPECT_EQ(provider_->GetString("missing", "default"), "default");
}

TEST_F(ConfigProviderTest, GenericGet) {
  provider_->Set("test", ConfigValue(42));
  auto result = provider_->Get("test", ConfigValue(0));
  EXPECT_TRUE(std::holds_alternative<int>(result));
  EXPECT_EQ(std::get<int>(result), 42);
}

TEST_F(ConfigProviderTest, SetAll) {
  std::vector<std::pair<std::string, ConfigValue>> values = {
      {"a", ConfigValue(1)},
      {"b", ConfigValue(2)},
      {"c", ConfigValue(std::string("three"))},
  };
  provider_->SetAll(values);

  EXPECT_EQ(provider_->GetInt("a", 0), 1);
  EXPECT_EQ(provider_->GetInt("b", 0), 2);
  EXPECT_EQ(provider_->GetString("c"), "three");
}

TEST_F(ConfigProviderTest, Persistence) {
  provider_->Set("persist.key", ConfigValue(123));
  provider_->Set("persist.name", ConfigValue(std::string("VEOR")));

  auto save_result = provider_->SaveNow();
  EXPECT_TRUE(save_result.IsOk());

  // Create a new provider and load
  auto provider2 = std::make_unique<ConfigProviderImpl>(factory_->GetIoRunner());
  provider2->SetFilePath(path_);
  auto load_result = provider2->Load();
  EXPECT_TRUE(load_result.IsOk());

  EXPECT_EQ(provider2->GetInt("persist.key", 0), 123);
  EXPECT_EQ(provider2->GetString("persist.name"), "VEOR");
}

TEST_F(ConfigProviderTest, ResetAll) {
  provider_->Set("a", ConfigValue(1));
  provider_->Set("b", ConfigValue(2));
  provider_->ResetAll();

  EXPECT_EQ(provider_->GetInt("a", 0), 0);
  EXPECT_EQ(provider_->GetInt("b", 0), 0);
}

TEST_F(ConfigProviderTest, LoadNonExistentFile) {
  provider_->SetFilePath("/nonexistent/path/config.json");
  auto result = provider_->Load();
  EXPECT_TRUE(result.IsOk());  // Non-existent file is not an error
}

TEST_F(ConfigProviderTest, OverwriteExistingValue) {
  provider_->Set("key", ConfigValue(1));
  EXPECT_EQ(provider_->GetInt("key", 0), 1);

  provider_->Set("key", ConfigValue(2));
  EXPECT_EQ(provider_->GetInt("key", 0), 2);
}

}  // namespace veor
