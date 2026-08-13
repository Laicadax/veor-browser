// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/VeorSandbox.h"

#include "core/logging/VeorLogger.h"

namespace veor {

bool InitializeSandbox() {
  // In modern Chromium, the sandbox is initialized automatically by the
  // content layer via the zygote (Linux), seatbelt (macOS), or broker
  // services (Windows). VEOR relies on ContentMain to set up the sandbox
  // before any renderer processes are spawned.
  //
  // This function exists as a hook for future custom sandbox policies.
  VEOR_LOGI(LogCategory::kCore, "Sandbox initialized by content layer");
  return true;
}

}  // namespace veor
