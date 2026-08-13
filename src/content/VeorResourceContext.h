// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/resource_context.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// VeorResourceContext
// ─────────────────────────────────────────────────────────────────────────────
// Minimal ResourceContext for VEOR. In modern Chromium (M90+), the network
// stack is managed by the Network Service, and ResourceContext is a legacy
// compatibility layer. VEOR delegates all network operations to the
// Network Service via content::StoragePartition.
// ─────────────────────────────────────────────────────────────────────────────

class VeorResourceContext : public content::ResourceContext {
 public:
  VeorResourceContext();
  ~VeorResourceContext() override;
};

}  // namespace veor
