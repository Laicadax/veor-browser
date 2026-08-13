// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/devtools_manager_delegate.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// VeorDevToolsDelegate
// ─────────────────────────────────────────────────────────────────────────────
// Minimal DevToolsManagerDelegate for VEOR.
// Provides default behavior for DevTools agent management.
// ─────────────────────────────────────────────────────────────────────────────

class VeorDevToolsDelegate : public content::DevToolsManagerDelegate {
 public:
  VeorDevToolsDelegate();
  ~VeorDevToolsDelegate() override;

  // content::DevToolsManagerDelegate
  void DevToolsAgentStateChanged(content::DevToolsAgentHost* agent_host,
                                 bool attached) override;
};

}  // namespace veor
