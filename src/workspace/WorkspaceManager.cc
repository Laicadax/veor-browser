// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "workspace/WorkspaceManager.h"

#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "core/logging/VeorLogger.h"
#include "workspace/SessionPersistence.h"

namespace veor {

WorkspaceManager::WorkspaceManager() = default;

WorkspaceManager::~WorkspaceManager() {
  if (persistence_) {
    PersistWorkspaces();
  }
}

Result<WorkspaceId, std::string> WorkspaceManager::CreateWorkspace(
    const WorkspaceCreationOptions& options) {
  auto id = IdGenerator::NextId<WorkspaceTag>();
  auto ws = std::make_unique<Workspace>(id, options.name);

  // Initialize per-workspace storage
  if (!persistence_base_path_.empty()) {
    base::FilePath ws_db = persistence_base_path_.Append(
        FILE_PATH_LITERAL("workspace_") +
        base::NumberToString(id.value()) +
        FILE_PATH_LITERAL(".db"));
    ws->InitializeStorage(ws_db);
  }

  if (options.activate_on_create) {
    ActivateWorkspace(id);
  }

  workspaces_[id] = std::move(ws);

  for (auto* observer : observers_) {
    observer->OnWorkspaceCreated(id);
  }

  VEOR_LOGI(LogCategory::kWorkspace,
            "Created workspace " + std::to_string(id.value()) + ": " +
                options.name);
  return Result<WorkspaceId, std::string>::Ok(id);
}

Result<void, std::string> WorkspaceManager::DeleteWorkspace(WorkspaceId id) {
  if (workspaces_.size() <= 1) {
    return Result<void, std::string>::Err(
        "Cannot delete the last workspace");
  }

  auto it = workspaces_.find(id);
  if (it == workspaces_.end()) {
    return Result<void, std::string>::Err("Workspace not found");
  }

  if (id == active_id_) {
    // Activate another workspace before deleting
    for (auto& [other_id, other_ws] : workspaces_) {
      if (other_id != id) {
        ActivateWorkspace(other_id);
        break;
      }
    }
  }

  workspaces_.erase(it);

  for (auto* observer : observers_) {
    observer->OnWorkspaceDeleted(id);
  }

  VEOR_LOGI(LogCategory::kWorkspace,
            "Deleted workspace " + std::to_string(id.value()));
  return Result<void, std::string>::Ok();
}

Result<WorkspaceId, std::string> WorkspaceManager::DuplicateWorkspace(
    WorkspaceId id,
    const std::string& new_name) {
  auto* src = GetWorkspace(id);
  if (!src) {
    return Result<WorkspaceId, std::string>::Err("Source workspace not found");
  }

  return CreateWorkspace({new_name, false, false});
}

Result<void, std::string> WorkspaceManager::ActivateWorkspace(WorkspaceId id) {
  auto it = workspaces_.find(id);
  if (it == workspaces_.end()) {
    return Result<void, std::string>::Err("Workspace not found");
  }

  // Deactivate current
  if (active_id_.IsValid()) {
    auto current = workspaces_.find(active_id_);
    if (current != workspaces_.end()) {
      current->second->Deactivate();
      for (auto* observer : observers_) {
        observer->OnWorkspaceDeactivated(active_id_);
      }
    }
  }

  // Activate new
  it->second->Activate();
  active_id_ = id;

  for (auto* observer : observers_) {
    observer->OnWorkspaceActivated(id);
  }

  VEOR_LOGI(LogCategory::kWorkspace,
            "Activated workspace " + std::to_string(id.value()));
  return Result<void, std::string>::Ok();
}

IWorkspace* WorkspaceManager::GetActiveWorkspace() const {
  return GetWorkspace(active_id_);
}

IWorkspace* WorkspaceManager::GetWorkspace(WorkspaceId id) const {
  auto it = workspaces_.find(id);
  return it != workspaces_.end() ? it->second.get() : nullptr;
}

std::vector<IWorkspace*> WorkspaceManager::GetAllWorkspaces() const {
  std::vector<IWorkspace*> result;
  result.reserve(workspaces_.size());
  for (const auto& [id, ws] : workspaces_) {
    result.push_back(ws.get());
  }
  return result;
}

size_t WorkspaceManager::GetWorkspaceCount() const {
  return workspaces_.size();
}

Result<void, std::string> WorkspaceManager::MoveTabToWorkspace(
    TabId tab,
    WorkspaceId target) {
  auto* target_ws = GetWorkspace(target);
  if (!target_ws) {
    return Result<void, std::string>::Err("Target workspace not found");
  }

  // Find which workspace owns this tab
  IWorkspace* source_ws = nullptr;
  TabInfo tab_info;
  for (auto& [id, ws] : workspaces_) {
    auto* tm = ws->GetTabManager();
    auto result = tm->GetTabInfo(tab);
    if (result.IsOk()) {
      source_ws = ws.get();
      tab_info = result.Value();
      break;
    }
  }

  if (!source_ws) {
    return Result<void, std::string>::Err("Tab not found in any workspace");
  }

  if (source_ws == target_ws) {
    return Result<void, std::string>::Ok();
  }

  // Close from source
  auto* src_tm = source_ws->GetTabManager();
  src_tm->CloseTab(tab);

  // Create in target with same URL
  auto* tgt_tm = target_ws->GetTabManager();
  auto new_tab = tgt_tm->CreateTab(tab_info.url);
  if (new_tab.IsOk()) {
    tgt_tm->SetTabTitle(new_tab.Value(), tab_info.title);
    VEOR_LOGI(LogCategory::kWorkspace,
              "Moved tab " + std::to_string(tab.value()) +
                  " to workspace " + std::to_string(target.value()));
  }

  return Result<void, std::string>::Ok();
}

Result<void, std::string> WorkspaceManager::PersistWorkspaces() {
  VEOR_LOGI(LogCategory::kWorkspace, "Persisting workspaces");

  if (!persistence_) {
    return Result<void, std::string>::Err("Persistence not initialized");
  }

  for (const auto& [id, ws] : workspaces_) {
    ws->SaveState();
    auto* session = ws->GetSession();
    if (session) {
      auto result = persistence_->SaveWorkspace(*session);
      if (result.IsErr()) {
        VEOR_LOGW(LogCategory::kWorkspace,
                  "Failed to persist workspace " + std::to_string(id.value()) +
                      ": " + result.UnwrapErr());
      }
    }
  }

  return Result<void, std::string>::Ok();
}

Result<void, std::string> WorkspaceManager::LoadWorkspaces() {
  VEOR_LOGI(LogCategory::kWorkspace, "Loading workspaces");

  if (!persistence_) {
    if (workspaces_.empty()) {
      CreateWorkspace({"Default", true, false});
    }
    return Result<void, std::string>::Ok();
  }

  auto sessions = persistence_->LoadAllWorkspaces();
  if (sessions.IsErr() || sessions.Value().empty()) {
    if (workspaces_.empty()) {
      CreateWorkspace({"Default", true, false});
    }
    return Result<void, std::string>::Ok();
  }

  for (const auto& session : sessions.Value()) {
    auto result = CreateWorkspace({session.GetName(), false, false});
    if (result.IsOk()) {
      auto* ws = static_cast<Workspace*>(GetWorkspace(result.Value()));
      if (ws) {
        *ws->GetSession() = session;
        ws->LoadState();
      }
    }
  }

  return Result<void, std::string>::Ok();
}

Result<void, std::string> WorkspaceManager::InitializePersistence(
    const base::FilePath& path) {
  persistence_base_path_ = path.DirName();
  persistence_ = std::make_unique<SessionPersistence>();
  auto result = persistence_->Open(path);
  if (result.IsOk()) {
    VEOR_LOGI(LogCategory::kWorkspace,
              "Persistence initialized at " + path.AsUTF8Unsafe());
  } else {
    VEOR_LOGW(LogCategory::kWorkspace,
              "Failed to initialize persistence: " + result.UnwrapErr());
  }
  return result;
}

void WorkspaceManager::AddObserver(WorkspaceObserver* observer) {
  observers_.push_back(observer);
}

void WorkspaceManager::RemoveObserver(WorkspaceObserver* observer) {
  observers_.erase(
      std::remove(observers_.begin(), observers_.end(), observer),
      observers_.end());
}

IWorkspaceManager* GetWorkspaceManager() {
  static WorkspaceManager* manager = new WorkspaceManager();
  return manager;
}

}  // namespace veor
