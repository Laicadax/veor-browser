// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/base/VeorId.h"

#include <atomic>

namespace veor {

std::atomic<uint64_t> IdGenerator::counter_{1};

uint64_t IdGenerator::Next() noexcept {
  return counter_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace veor
