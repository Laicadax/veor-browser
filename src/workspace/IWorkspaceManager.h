// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
class Workspace;

#include <string>
#include <vector>

#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"
#include "workspace/IWorkspace.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// WorkspaceCreationOptions
// ─────────────────────────────────────────────────────────────────────────────

struct WorkspaceCreationOptions {
  std::string name;
  bool activate_on_create = true;
  bool restore_from_last_session = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// IWorkspaceManager
// ─────────────────────────────────────────────────────────────────────────────

class IWorkspaceManager {
 public:
  virtual ~IWorkspaceManager() = default;

  // CRUD
  virtual Result<WorkspaceId, std::string> CreateWorkspace(
      const WorkspaceCreationOptions& options) = 0;
  virtual Result<void, std::string> DeleteWorkspace(WorkspaceId id) = 0;
  virtual Result<WorkspaceId, std::string> DuplicateWorkspace(
      WorkspaceId id, const std::string& new_name) = 0;

  // Activation
  virtual Result<void, std::string> ActivateWorkspace(WorkspaceId id) = 0;
  virtual WorkspaceId GetActiveWorkspaceId() const = 0;
  virtual IWorkspace* GetActiveWorkspace() const = 0;

  // Queries
  virtual IWorkspace* GetWorkspace(WorkspaceId id) const = 0;
  virtual std::vector<IWorkspace*> GetAllWorkspaces() const = 0;
  virtual size_t GetWorkspaceCount() const = 0;

  // Tab Movement
  virtual Result<void, std::string> MoveTabToWorkspace(TabId tab,
                                                       WorkspaceId target) = 0;

  // Persistence
  virtual Result<void, std::string> PersistWorkspaces() = 0;
  virtual Result<void, std::string> LoadWorkspaces() = 0;

  // Observers
  virtual void AddObserver(WorkspaceObserver* observer) = 0;
  virtual void RemoveObserver(WorkspaceObserver* observer) = 0;
};

// Global accessor
IWorkspaceManager* GetWorkspaceManager();

}  // namespace veor
