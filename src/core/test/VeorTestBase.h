// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "testing/gtest/include/gtest/gtest.h"

namespace veor {

// Base test fixture that initializes and shuts down the Core layer
// automatically for each test.
class VeorTestBase : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;
};

}  // namespace veor
