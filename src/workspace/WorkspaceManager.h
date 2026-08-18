// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
class Workspace;

#include "workspace/IWorkspaceManager.h"
#include "base/files/file_path.h"

#include <unordered_map>

namespace veor {

class SessionPersistence;

// ─────────────────────────────────────────────────────────────────────────────
// WorkspaceManager
// ─────────────────────────────────────────────────────────────────────────────

class WorkspaceManager : public IWorkspaceManager {
 public:
  WorkspaceManager();
  ~WorkspaceManager() override;

  // IWorkspaceManager
  Result<WorkspaceId, std::string> CreateWorkspace(
      const WorkspaceCreationOptions& options) override;
  Result<void, std::string> DeleteWorkspace(WorkspaceId id) override;
  Result<WorkspaceId, std::string> DuplicateWorkspace(
      WorkspaceId id, const std::string& new_name) override;

  Result<void, std::string> ActivateWorkspace(WorkspaceId id) override;
  WorkspaceId GetActiveWorkspaceId() const override { return active_id_; }
  IWorkspace* GetActiveWorkspace() const override;

  IWorkspace* GetWorkspace(WorkspaceId id) const override;
  std::vector<IWorkspace*> GetAllWorkspaces() const override;
  size_t GetWorkspaceCount() const override;

  Result<void, std::string> MoveTabToWorkspace(TabId tab,
                                               WorkspaceId target) override;

  Result<void, std::string> PersistWorkspaces() override;
  Result<void, std::string> LoadWorkspaces() override;

  void AddObserver(WorkspaceObserver* observer) override;
  void RemoveObserver(WorkspaceObserver* observer) override;

  // Initializes the session persistence layer.
  // Must be called before PersistWorkspaces / LoadWorkspaces.
  Result<void, std::string> InitializePersistence(const base::FilePath& path);

 private:
  std::unordered_map<WorkspaceId, std::unique_ptr<Workspace>,
                     WorkspaceId::Hash>
      workspaces_;
  WorkspaceId active_id_;
  std::vector<WorkspaceObserver*> observers_;
  std::unique_ptr<SessionPersistence> persistence_;
  base::FilePath persistence_base_path_;
};

}  // namespace veor
