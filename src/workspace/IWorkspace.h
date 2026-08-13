// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/time/time.h"
#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"

namespace veor {

// Forward declarations
class ITabManager;
class IHistoryStore;
class IBookmarkStore;
class ISettingsProvider;
class WorkspaceSession;

// ─────────────────────────────────────────────────────────────────────────────
// WorkspaceObserver
// ─────────────────────────────────────────────────────────────────────────────

class WorkspaceObserver {
 public:
  virtual ~WorkspaceObserver() = default;

  virtual void OnWorkspaceCreated(WorkspaceId id) {}
  virtual void OnWorkspaceDeleted(WorkspaceId id) {}
  virtual void OnWorkspaceActivated(WorkspaceId id) {}
  virtual void OnWorkspaceDeactivated(WorkspaceId id) {}
  virtual void OnWorkspaceRenamed(WorkspaceId id,
                                   const std::string& new_name) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// IWorkspace
// ─────────────────────────────────────────────────────────────────────────────

class IWorkspace {
 public:
  virtual ~IWorkspace() = default;

  // Identity
  virtual WorkspaceId GetId() const = 0;
  virtual const std::string& GetName() const = 0;
  virtual void SetName(const std::string& name) = 0;

  // State
  virtual bool IsActive() const = 0;
  virtual void Activate() = 0;
  virtual void Deactivate() = 0;

  // Subsystems
  virtual ITabManager* GetTabManager() const = 0;
  virtual IHistoryStore* GetHistoryStore() const = 0;
  virtual IBookmarkStore* GetBookmarkStore() const = 0;
  virtual ISettingsProvider* GetSettings() const = 0;
  virtual WorkspaceSession* GetSession() const = 0;

  // Persistence
  virtual Result<void, std::string> SaveState() = 0;
  virtual Result<void, std::string> LoadState() = 0;

  // Metrics
  virtual base::Time GetLastActivatedTime() const = 0;
  virtual size_t GetMemoryUsage() const = 0;
};

}  // namespace veor
