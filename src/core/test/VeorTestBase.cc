// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/test/VeorTestBase.h"

#include "core/CoreInit.h"

namespace veor {

void VeorTestBase::SetUp() {
  CoreInitOptions opts;
  opts.min_log_level = LogLevel::kWarning;
  auto result = CoreInit::Initialize(opts);
  ASSERT_TRUE(result.IsOk()) << result.UnwrapErr();
}

void VeorTestBase::TearDown() {
  CoreInit::Shutdown();
}

}  // namespace veor
