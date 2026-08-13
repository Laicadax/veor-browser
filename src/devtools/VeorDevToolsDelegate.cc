// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools/VeorDevToolsDelegate.h"

namespace veor {

VeorDevToolsDelegate::VeorDevToolsDelegate() = default;
VeorDevToolsDelegate::~VeorDevToolsDelegate() = default;

void VeorDevToolsDelegate::DevToolsAgentStateChanged(
    content::DevToolsAgentHost* agent_host,
    bool attached) {
  // No-op for MVP
}

}  // namespace veor
