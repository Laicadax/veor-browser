// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unordered_map>
#include <vector>

#include "tabs/ITabManager.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// Tab — Internal tab state container
// ─────────────────────────────────────────────────────────────────────────────

struct Tab {
  TabId id;
  TabState state = TabState::kBorn;
  GURL url;
  std::string title;
  bool pinned = false;
  bool active = false;
  TabGroupId group = kNoGroup;
};

// ─────────────────────────────────────────────────────────────────────────────
// TabManager
// ─────────────────────────────────────────────────────────────────────────────

class TabManager : public ITabManager {
 public:
  TabManager() = default;
  ~TabManager() override = default;

  // ITabManager
  Result<TabId, std::string> CreateTab(const TabCreationOptions& options) override;
  Result<void, std::string> CloseTab(TabId id) override;

  Result<void, std::string> ActivateTab(TabId id) override;
  TabId GetActiveTabId() const override { return active_tab_; }

  Result<void, std::string> SleepTab(TabId id) override;
  Result<void, std::string> WakeTab(TabId id) override;

  Result<void, std::string> PinTab(TabId id, bool pinned) override;

  Result<void, std::string> NavigateTab(TabId id, const GURL& url) override;

  std::vector<TabInfo> GetAllTabs() const override;

  Result<void, std::string> SetTabTitle(TabId id,
                                         const std::string& title) override;

  void AddObserver(TabObserver* observer) override;
  void RemoveObserver(TabObserver* observer) override;

 private:
  std::unordered_map<TabId, Tab, TabId::Hash> tabs_;
  TabId active_tab_;
  std::vector<TabObserver*> observers_;
};

}  // namespace veor
