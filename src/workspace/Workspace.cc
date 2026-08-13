// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "workspace/Workspace.h"

#include "base/time/time.h"
#include "bookmarks/BookmarkStoreImpl.h"
#include "bookmarks/BookmarkStoreStub.h"
#include "core/logging/VeorLogger.h"
#include "history/HistoryStoreImpl.h"
#include "history/HistoryStoreStub.h"
#include "infrastructure/storage/StorageEngineImpl.h"
#include "settings/SettingsProviderImpl.h"
#include "settings/SettingsProviderStub.h"
#include "tabs/TabManager.h"
#include "workspace/WorkspaceSession.h"

namespace veor {

Workspace::Workspace(WorkspaceId id, const std::string& name)
    : id_(id), name_(name) {
  tab_manager_ = std::make_unique<TabManager>();
  session_ = std::make_unique<WorkspaceSession>();
  // Stores are initialized via InitializeStorage()
}

void Workspace::SetName(const std::string& name) {
  name_ = name;
  VEOR_LOGI(LogCategory::kWorkspace,
            "Workspace " + std::to_string(id_.value()) + " renamed to " + name);
}

void Workspace::Activate() {
  is_active_ = true;
  last_activated_ = base::Time::Now();
}

void Workspace::Deactivate() {
  is_active_ = false;
}

ITabManager* Workspace::GetTabManager() const {
  return tab_manager_.get();
}

IHistoryStore* Workspace::GetHistoryStore() const {
  return history_store_.get();
}

IBookmarkStore* Workspace::GetBookmarkStore() const {
  return bookmark_store_.get();
}

ISettingsProvider* Workspace::GetSettings() const {
  return settings_.get();
}

WorkspaceSession* Workspace::GetSession() const {
  return session_.get();
}

Result<void, std::string> Workspace::SaveState() {
  session_->ClearTabs();
  session_->SetWorkspaceId(id_);
  session_->SetName(name_);
  session_->SetLastActiveTime(last_activated_);

  auto tabs = tab_manager_->GetAllTabs();
  for (const auto& tab : tabs) {
    TabSession ts;
    ts.tab_id = tab.id;
    ts.url = tab.url;
    ts.title = tab.title;
    ts.pinned = tab.pinned;
    ts.active = (tab.id == tab_manager_->GetActiveTabId());
    session_->AddTab(ts);
  }

  VEOR_LOGD(LogCategory::kWorkspace,
            "Workspace " + std::to_string(id_.value()) + " state serialized: " +
                std::to_string(tabs.size()) + " tabs");
  return Result<void, std::string>::Ok();
}

Result<void, std::string> Workspace::LoadState() {
  if (session_->GetTabs().empty()) {
    return Result<void, std::string>::Ok();
  }

  // Clear existing tabs
  for (const auto& tab : tab_manager_->GetAllTabs()) {
    tab_manager_->CloseTab(tab.id);
  }

  // Restore tabs from session
  for (const auto& tab : session_->GetTabs()) {
    TabCreationOptions options;
    options.url = tab.url;
    options.pinned = tab.pinned;
    options.activate = tab.active;
    auto result = tab_manager_->CreateTab(options);
    if (result.IsOk()) {
      TabId id = result.Unwrap();
      tab_manager_->SetTabTitle(id, tab.title);
      if (tab.pinned) {
        tab_manager_->PinTab(id, true);
      }
      if (tab.active) {
        tab_manager_->ActivateTab(id);
      }
    }
  }

  VEOR_LOGD(LogCategory::kWorkspace,
            "Workspace " + std::to_string(id_.value()) + " state restored: " +
                std::to_string(session_->GetTabs().size()) + " tabs");
  return Result<void, std::string>::Ok();
}

size_t Workspace::GetMemoryUsage() const {
  // Approximate: 2KB per tab + stores
  size_t tab_memory = tab_manager_->GetAllTabs().size() * 2048;
  return tab_memory + 65536;  // Base overhead
}

void Workspace::InitializeStorage(const base::FilePath& db_path) {
  storage_engine_ = std::make_unique<StorageEngineImpl>();
  auto result = storage_engine_->Open(db_path);
  if (result.IsOk()) {
    history_store_ = std::make_unique<HistoryStoreImpl>(storage_engine_.get());
    bookmark_store_ = std::make_unique<BookmarkStoreImpl>(storage_engine_.get());
    VEOR_LOGI(LogCategory::kWorkspace,
              "Storage initialized for workspace " + std::to_string(id_.value()));
  } else {
    VEOR_LOGW(LogCategory::kWorkspace,
              "Failed to open workspace storage, using stubs: " +
              result.UnwrapErr().ToString());
    history_store_ = std::make_unique<HistoryStoreStub>();
    bookmark_store_ = std::make_unique<BookmarkStoreStub>();
  }
}

}  // namespace veor
