// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "workspace/SessionPersistence.h"

#include "core/logging/VeorLogger.h"
#include "infrastructure/storage/StorageEngineImpl.h"

namespace veor {

namespace {

constexpr int kSchemaVersion = 1;

}  // namespace

SessionPersistence::SessionPersistence()
    : engine_(std::make_unique<StorageEngineImpl>()) {}

SessionPersistence::~SessionPersistence() {
  if (is_open_) {
    Close();
  }
}

Result<void, std::string> SessionPersistence::Open(const base::FilePath& path) {
  auto result = engine_->Open(path);
  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }

  is_open_ = true;

  auto schema_result = EnsureSchema();
  if (schema_result.IsErr()) {
    return schema_result;
  }

  VEOR_LOGI(LogCategory::kWorkspace,
            "SessionPersistence opened: " + path.value());
  return Result<void, std::string>::Ok();
}

Result<void, std::string> SessionPersistence::Close() {
  if (!is_open_) {
    return Result<void, std::string>::Ok();
  }

  auto result = engine_->Close();
  is_open_ = false;

  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }

  return Result<void, std::string>::Ok();
}

Result<void, std::string> SessionPersistence::EnsureSchema() {
  int version = engine_->GetVersion();
  if (version == 0) {
    // Fresh database — create schema
    auto create_result = engine_->Execute(R"sql(
      CREATE TABLE IF NOT EXISTS workspaces (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        last_active INTEGER NOT NULL
      );
      CREATE TABLE IF NOT EXISTS tabs (
        id INTEGER PRIMARY KEY,
        workspace_id INTEGER NOT NULL,
        url TEXT NOT NULL,
        title TEXT NOT NULL,
        pinned INTEGER NOT NULL DEFAULT 0,
        active INTEGER NOT NULL DEFAULT 0,
        sort_index INTEGER NOT NULL DEFAULT 0,
        FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE
      );
      CREATE INDEX IF NOT EXISTS idx_tabs_workspace ON tabs(workspace_id);
    )sql");

    if (create_result.IsErr()) {
      return Result<void, std::string>::Err(
          create_result.UnwrapErr().ToString());
    }

    auto migrate_result = engine_->Migrate(kSchemaVersion);
    if (migrate_result.IsErr()) {
      return Result<void, std::string>::Err(
          migrate_result.UnwrapErr().ToString());
    }
  } else if (version != kSchemaVersion) {
    return Result<void, std::string>::Err(
        "Unsupported schema version: " + std::to_string(version));
  }

  return Result<void, std::string>::Ok();
}

Result<void, std::string> SessionPersistence::SaveWorkspace(
    const WorkspaceSession& session) {
  if (!is_open_) {
    return Result<void, std::string>::Err("Database not open");
  }

  auto txn_result = engine_->BeginTransaction();
  if (txn_result.IsErr()) {
    return Result<void, std::string>::Err(txn_result.UnwrapErr().ToString());
  }
  auto txn = std::move(txn_result).Unwrap();

  // Delete existing tabs for this workspace
  auto del_result = engine_->Execute(
      "DELETE FROM tabs WHERE workspace_id = ?",
      {std::to_string(session.GetWorkspaceId().value())});
  if (del_result.IsErr()) {
    txn->Rollback();
    return Result<void, std::string>::Err(del_result.UnwrapErr().ToString());
  }

  // Upsert workspace
  auto upsert_result = engine_->Execute(
      "INSERT OR REPLACE INTO workspaces (id, name, last_active) VALUES (?, ?, ?)",
      {std::to_string(session.GetWorkspaceId().value()),
       session.GetName(),
       std::to_string(session.GetLastActiveTime().ToTimeT())});
  if (upsert_result.IsErr()) {
    txn->Rollback();
    return Result<void, std::string>::Err(upsert_result.UnwrapErr().ToString());
  }

  // Insert tabs
  const auto& tabs = session.GetTabs();
  for (size_t i = 0; i < tabs.size(); ++i) {
    const auto& tab = tabs[i];
    auto tab_result = engine_->Execute(
        "INSERT INTO tabs (workspace_id, url, title, pinned, active, sort_index) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {std::to_string(session.GetWorkspaceId().value()),
         tab.url.spec(),
         tab.title,
         tab.pinned ? "1" : "0",
         tab.active ? "1" : "0",
         std::to_string(i)});
    if (tab_result.IsErr()) {
      txn->Rollback();
      return Result<void, std::string>::Err(tab_result.UnwrapErr().ToString());
    }
  }

  auto commit_result = txn->Commit();
  if (commit_result.IsErr()) {
    return Result<void, std::string>::Err(commit_result.UnwrapErr().ToString());
  }

  return Result<void, std::string>::Ok();
}

Result<std::vector<WorkspaceSession>, std::string>
SessionPersistence::LoadAllWorkspaces() {
  if (!is_open_) {
    return Result<std::vector<WorkspaceSession>, std::string>::Err(
        "Database not open");
  }

  std::vector<WorkspaceSession> sessions;

  // Load workspaces
  auto ws_stmt_result = engine_->Prepare(
      "SELECT id, name, last_active FROM workspaces ORDER BY last_active DESC");
  if (ws_stmt_result.IsErr()) {
    return Result<std::vector<WorkspaceSession>, std::string>::Err(
        ws_stmt_result.UnwrapErr().ToString());
  }
  auto ws_stmt = std::move(ws_stmt_result).Unwrap();

  while (true) {
    auto step_result = ws_stmt->Step();
    if (step_result.IsErr()) {
      return Result<std::vector<WorkspaceSession>, std::string>::Err(
          step_result.UnwrapErr().ToString());
    }
    if (!step_result.Value()) {
      break;
    }

    WorkspaceSession session;
    session.SetWorkspaceId(WorkspaceId(ws_stmt->GetInt64(0)));
    session.SetName(ws_stmt->GetString(1));
    session.SetLastActiveTime(
        base::Time::FromTimeT(static_cast<time_t>(ws_stmt->GetInt64(2))));

    // Load tabs for this workspace
    auto tab_stmt_result = engine_->Prepare(
        "SELECT url, title, pinned, active FROM tabs WHERE workspace_id = ? "
        "ORDER BY sort_index ASC");
    if (tab_stmt_result.IsErr()) {
      return Result<std::vector<WorkspaceSession>, std::string>::Err(
          tab_stmt_result.UnwrapErr().ToString());
    }
    auto tab_stmt = std::move(tab_stmt_result).Unwrap();
    tab_stmt->BindInt64(1, session.GetWorkspaceId().value());

    while (true) {
      auto tab_step = tab_stmt->Step();
      if (tab_step.IsErr()) {
        return Result<std::vector<WorkspaceSession>, std::string>::Err(
            tab_step.UnwrapErr().ToString());
      }
      if (!tab_step.Value()) {
        break;
      }

      TabSession tab;
      tab.url = GURL(tab_stmt->GetString(0));
      tab.title = tab_stmt->GetString(1);
      tab.pinned = tab_stmt->GetInt(2) != 0;
      tab.active = tab_stmt->GetInt(3) != 0;
      session.AddTab(tab);
    }

    sessions.push_back(std::move(session));
  }

  return Result<std::vector<WorkspaceSession>, std::string>::Ok(
      std::move(sessions));
}

Result<void, std::string> SessionPersistence::DeleteWorkspace(WorkspaceId id) {
  if (!is_open_) {
    return Result<void, std::string>::Err("Database not open");
  }

  auto result = engine_->Execute("DELETE FROM workspaces WHERE id = ?",
                                  {std::to_string(id.value())});
  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }

  return Result<void, std::string>::Ok();
}

Result<void, std::string> SessionPersistence::ClearAll() {
  if (!is_open_) {
    return Result<void, std::string>::Err("Database not open");
  }

  auto result = engine_->Execute(
      "DELETE FROM tabs; DELETE FROM workspaces;");
  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }

  return Result<void, std::string>::Ok();
}

}  // namespace veor
