// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "infrastructure/platform/PlatformServiceImpl.h"
#include "core/test/VeorTestBase.h"

namespace veor {

class PlatformServiceTest : public VeorTestBase {
 protected:
  void SetUp() override {
    VeorTestBase::SetUp();
    platform_ = std::make_unique<PlatformServiceImpl>();
  }

  std::unique_ptr<PlatformServiceImpl> platform_;
};

TEST_F(PlatformServiceTest, GetUserDataDirectory) {
  auto path = platform_->GetUserDataDirectory();
  EXPECT_FALSE(path.empty());
  EXPECT_TRUE(path.EndsWith(FILE_PATH_LITERAL("VEOR")));
}

TEST_F(PlatformServiceTest, GetCacheDirectory) {
  auto path = platform_->GetCacheDirectory();
  EXPECT_FALSE(path.empty());
  EXPECT_TRUE(path.EndsWith(FILE_PATH_LITERAL("Cache")));
}

TEST_F(PlatformServiceTest, GetDownloadsDirectory) {
  auto path = platform_->GetDownloadsDirectory();
  EXPECT_FALSE(path.empty());
}

TEST_F(PlatformServiceTest, GetTempDirectory) {
  auto path = platform_->GetTempDirectory();
  EXPECT_FALSE(path.empty());
  EXPECT_TRUE(path.EndsWith(FILE_PATH_LITERAL("veor")));
}

TEST_F(PlatformServiceTest, HardwareInfo) {
  int cpus = platform_->GetLogicalCpuCount();
  EXPECT_GT(cpus, 0);

  size_t memory = platform_->GetTotalPhysicalMemory();
  EXPECT_GT(memory, 0u);
}

TEST_F(PlatformServiceTest, NotificationDoesNotCrash) {
  // Notifications are stubbed; just verify no crash.
  platform_->ShowNotification("Test", "Body", base::DoNothing());
}

TEST_F(PlatformServiceTest, BadgeCountDoesNotCrash) {
  platform_->SetBadgeCount(5);
}

}  // namespace veor
