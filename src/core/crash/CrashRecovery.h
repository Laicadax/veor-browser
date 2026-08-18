// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
class Workspace;

#include <string>
#include <vector>

#include "core/base/VeorResult.h"
#include "workspace/IWorkspaceManager.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SessionSnapshot — Serializable browser state for crash recovery
// ─────────────────────────────────────────────────────────────────────────────

struct SessionSnapshot {
  struct WorkspaceSnapshot {
    std::string name;
    std::vector<std::string> tab_urls;
    int active_tab_index = 0;
  };

  std::vector<WorkspaceSnapshot> workspaces;
  int active_workspace_index = 0;
  int64_t timestamp_ms = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// CrashRecovery — Session restore after crash
// ─────────────────────────────────────────────────────────────────────────────

class CrashRecovery {
 public:
  CrashRecovery();
  ~CrashRecovery() = default;

  // Save current session state
  Result<void, std::string> SaveSnapshot(const SessionSnapshot& snapshot);

  // Load last saved snapshot
  Result<SessionSnapshot, std::string> LoadSnapshot();

  // Check if previous session crashed (snapshot exists but clean exit flag not set)
  bool HasCrashedSession() const;

  // Mark current session as clean (call on graceful shutdown)
  Result<void, std::string> MarkCleanExit();

  // Restore workspaces from snapshot
  Result<void, std::string> RestoreSession(IWorkspaceManager* manager);

 private:
  std::string GetSnapshotPath() const;
  std::string GetCleanExitPath() const;
};

}  // namespace veor
