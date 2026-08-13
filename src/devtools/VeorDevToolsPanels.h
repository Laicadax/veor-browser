// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/devtools_agent_host.h"

namespace veor {

class MemoryTracker;
class WorkspaceManager;
class ISyncEngine;

// ─────────────────────────────────────────────────────────────────────────────
// VeorDevToolsPanels
// ─────────────────────────────────────────────────────────────────────────────
// Injects custom VEOR panels into Chrome DevTools:
//   - Memory: live memory stats from MemoryTracker
//   - Workspaces: active workspaces, tabs, session info
//   - Sync: sync engine status
//
// Uses Chrome DevTools Protocol (CDP) to create panels via
// Runtime.evaluate and Inspector experiments.

class VeorDevToolsPanels {
 public:
  VeorDevToolsPanels(MemoryTracker* memory,
                     WorkspaceManager* workspaces,
                     ISyncEngine* sync);
  ~VeorDevToolsPanels() = default;

  // Called when DevTools opens for a WebContents.
  void OnDevToolsOpened(content::DevToolsAgentHost* agent_host);

  // Called periodically to update panel data.
  void UpdatePanels();

 private:
  void InjectMemoryPanel(content::DevToolsAgentHost* host);
  void InjectWorkspacePanel(content::DevToolsAgentHost* host);
  void InjectSyncPanel(content::DevToolsAgentHost* host);

  std::string BuildMemoryPayload();
  std::string BuildWorkspacePayload();
  std::string BuildSyncPayload();

  MemoryTracker* memory_;
  WorkspaceManager* workspaces_;
  ISyncEngine* sync_;

  base::WeakPtrFactory<VeorDevToolsPanels> weak_factory_{this};
};

}  // namespace veor
