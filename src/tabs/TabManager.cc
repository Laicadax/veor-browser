// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tabs/TabManager.h"
#include "url/gurl.h"

#include <algorithm>

#include "core/base/VeorId.h"

namespace veor {

Result<TabId, std::string> TabManager::CreateTab(const TabCreationOptions& options) {
  TabId id = IdGenerator::NextId<TabTag>();
  Tab tab;
  tab.id = id;
  tab.url = options.url;
  tab.pinned = options.pinned;
  tab.group = options.group;
  tabs_[id] = std::move(tab);

  if (options.activate) {
    ActivateTab(id);
  }

  for (auto* o : observers_) {
    o->OnTabCreated(id);
  }
  return Result<TabId, std::string>::Ok(id);
}

Result<void, std::string> TabManager::CloseTab(TabId id) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }

  for (auto* o : observers_) {
    o->OnTabClosing(id);
  }

  tabs_.erase(it);

  for (auto* o : observers_) {
    o->OnTabClosed(id);
  }
  return Result<void, std::string>::Ok();
}

Result<void, std::string> TabManager::ActivateTab(TabId id) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }

  if (active_tab_.IsValid()) {
    auto old = tabs_.find(active_tab_);
    if (old != tabs_.end()) {
      old->second.active = false;
    }
  }

  it->second.active = true;
  active_tab_ = id;

  for (auto* o : observers_) {
    o->OnTabActivated(id);
  }
  return Result<void, std::string>::Ok();
}

Result<void, std::string> TabManager::SleepTab(TabId id) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }
  it->second.state = TabState::kSleeping;
  return Result<void, std::string>::Ok();
}

Result<void, std::string> TabManager::WakeTab(TabId id) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }
  it->second.state = TabState::kBackground;
  return Result<void, std::string>::Ok();
}

Result<void, std::string> TabManager::PinTab(TabId id, bool pinned) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }
  it->second.pinned = pinned;
  return Result<void, std::string>::Ok();
}

Result<void, std::string> TabManager::NavigateTab(TabId id, const GURL& url) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }
  it->second.url = url;
  for (auto* o : observers_) {
    o->OnTabUrlChanged(id, url);
  }
  return Result<void, std::string>::Ok();
}

void TabManager::AddObserver(TabObserver* observer) {
  observers_.push_back(observer);
}

void TabManager::RemoveObserver(TabObserver* observer) {
  observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                   observers_.end());
}

std::vector<TabInfo> TabManager::GetAllTabs() const {
  std::vector<TabInfo> result;
  result.reserve(tabs_.size());
  for (const auto& [id, tab] : tabs_) {
    TabInfo info;
    info.id = tab.id;
    info.state = tab.state;
    info.url = tab.url;
    info.title = tab.title;
    info.pinned = tab.pinned;
    info.active = tab.active;
    info.group = tab.group;
    result.push_back(info);
  }
  return result;
}

Result<void, std::string> TabManager::SetTabTitle(TabId id,
                                                   const std::string& title) {
  auto it = tabs_.find(id);
  if (it == tabs_.end()) {
    return Result<void, std::string>::Err("Tab not found");
  }
  it->second.title = title;
  for (auto* o : observers_) {
    o->OnTabTitleChanged(id, title);
  }
  return Result<void, std::string>::Ok();
}

}  // namespace veor
