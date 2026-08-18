// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
class Workspace;

#include "base/time/time.h"
#include "base/files/file_path.h"
#include "workspace/IWorkspace.h"

#include <memory>

namespace veor {

class IStorageEngine;

// ─────────────────────────────────────────────────────────────────────────────
// Workspace
// ─────────────────────────────────────────────────────────────────────────────

class Workspace : public IWorkspace {
 public:
  Workspace(WorkspaceId id, const std::string& name);
  ~Workspace() override = default;

  // IWorkspace
  WorkspaceId GetId() const override { return id_; }
  const std::string& GetName() const override { return name_; }
  void SetName(const std::string& name) override;

  bool IsActive() const override { return is_active_; }
  void Activate() override;
  void Deactivate() override;

  ITabManager* GetTabManager() const override;
  IHistoryStore* GetHistoryStore() const override;
  IBookmarkStore* GetBookmarkStore() const override;
  ISettingsProvider* GetSettings() const override;
  WorkspaceSession* GetSession() const override;

  Result<void, std::string> SaveState() override;
  Result<void, std::string> LoadState() override;

  base::Time GetLastActivatedTime() const override { return last_activated_; }
  size_t GetMemoryUsage() const override;

  // Initialize SQLite-backed stores for this workspace.
  void InitializeStorage(const base::FilePath& db_path);

 private:
  WorkspaceId id_;
  std::string name_;
  bool is_active_ = false;
  base::Time last_activated_;

  std::unique_ptr<ITabManager> tab_manager_;
  std::unique_ptr<IHistoryStore> history_store_;
  std::unique_ptr<IBookmarkStore> bookmark_store_;
  std::unique_ptr<ISettingsProvider> settings_;
  std::unique_ptr<WorkspaceSession> session_;
  std::unique_ptr<IStorageEngine> storage_engine_;
};

}  // namespace veor
