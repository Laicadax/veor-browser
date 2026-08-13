// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// TabState
// ─────────────────────────────────────────────────────────────────────────────

enum class TabState {
  kBorn,        // Created, no WebContents
  kLoaded,      // WebContents exists, content loading
  kActive,      // Visible and focused
  kBackground,  // Loaded but not visible
  kPinned,      // Pinned, always loaded
  kSleeping,    // WebContents unloaded, state preserved
  kClosing,     // In process of closing
  kClosed       // Fully closed, awaiting cleanup
};

// ─────────────────────────────────────────────────────────────────────────────
// TabObserver
// ─────────────────────────────────────────────────────────────────────────────

class TabObserver {
 public:
  virtual ~TabObserver() = default;

  virtual void OnTabCreated(TabId id) {}
  virtual void OnTabStateChanged(TabId id, TabState old_state, TabState new_state) {}
  virtual void OnTabTitleChanged(TabId id, const std::string& title) {}
  virtual void OnTabUrlChanged(TabId id, const GURL& url) {}
  virtual void OnTabFaviconChanged(TabId id, const std::vector<uint8_t>& favicon) {}
  virtual void OnTabActivated(TabId id) {}
  virtual void OnTabClosing(TabId id) {}
  virtual void OnTabClosed(TabId id) {}
  virtual void OnTabPinnedChanged(TabId id, bool pinned) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// TabCreationOptions
// ─────────────────────────────────────────────────────────────────────────────

struct TabCreationOptions {
  GURL url;
  TabGroupId group = kNoGroup;
  bool activate = true;
  bool pinned = false;
  int insert_index = -1;  // -1 = append to end
};

// ─────────────────────────────────────────────────────────────────────────────
// ITabManager
// ─────────────────────────────────────────────────────────────────────────────

class ITabManager {
 public:
  virtual ~ITabManager() = default;

  // CRUD
  virtual Result<TabId, std::string> CreateTab(const TabCreationOptions& options) = 0;
  virtual Result<void, std::string> CloseTab(TabId id) = 0;

  // Activation
  virtual Result<void, std::string> ActivateTab(TabId id) = 0;
  virtual TabId GetActiveTabId() const = 0;

  // Sleep / Wake
  virtual Result<void, std::string> SleepTab(TabId id) = 0;
  virtual Result<void, std::string> WakeTab(TabId id) = 0;

  // Pinning
  virtual Result<void, std::string> PinTab(TabId id, bool pinned) = 0;

  // Navigation
  virtual Result<void, std::string> NavigateTab(TabId id, const GURL& url) = 0;

  // Metadata
  virtual Result<void, std::string> SetTabTitle(TabId id,
                                                 const std::string& title) = 0;

  // Queries
  virtual std::vector<TabInfo> GetAllTabs() const = 0;

  // Observers
  virtual void AddObserver(TabObserver* observer) = 0;
  virtual void RemoveObserver(TabObserver* observer) = 0;
};

}  // namespace veor
