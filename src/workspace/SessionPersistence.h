// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "core/base/VeorResult.h"
#include "infrastructure/storage/IStorageEngine.h"
#include "workspace/WorkspaceSession.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SessionPersistence
// ─────────────────────────────────────────────────────────────────────────────
// Persists workspace sessions to SQLite using StorageEngine.
// Schema:
//   workspaces(id INTEGER PRIMARY KEY, name TEXT, last_active INTEGER)
//   tabs(id INTEGER PRIMARY KEY, workspace_id INTEGER, url TEXT,
//        title TEXT, pinned INTEGER, active INTEGER, sort_index INTEGER)
//
// Thread safety: [IO Thread] for all methods.

class SessionPersistence {
 public:
  SessionPersistence();
  ~SessionPersistence();

  // Opens the session database at the given path.
  // Creates tables if they don't exist.
  Result<void, std::string> Open(const base::FilePath& path);

  // Closes the database.
  Result<void, std::string> Close();

  // Persists a workspace session.
  Result<void, std::string> SaveWorkspace(const WorkspaceSession& session);

  // Loads all workspace sessions.
  Result<std::vector<WorkspaceSession>, std::string> LoadAllWorkspaces();

  // Deletes a workspace and its tabs.
  Result<void, std::string> DeleteWorkspace(WorkspaceId id);

  // Clears all data.
  Result<void, std::string> ClearAll();

 private:
  Result<void, std::string> EnsureSchema();

  std::unique_ptr<IStorageEngine> engine_;
  bool is_open_ = false;
};

}  // namespace veor
