// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// InitializeSandbox
// ─────────────────────────────────────────────────────────────────────────────
// Initializes the platform sandbox before any child processes start.
// Must be called from ContentMainDelegate::PreSandboxStartup().
//
// Linux: seccomp-bpf (SandboxBPF)
// macOS: Seatbelt
// Windows: Sandbox broker
//
// Returns true on success. Failure is fatal — browser exits.
// ─────────────────────────────────────────────────────────────────────────────

bool InitializeSandbox();

}  // namespace veor
