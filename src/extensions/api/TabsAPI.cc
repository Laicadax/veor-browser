// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/api/TabsAPI.h"

#include "core/base/UrlSecurity.h"
#include "core/logging/VeorLogger.h"

namespace veor {

TabsAPI::TabsAPI(ITabManager* tab_manager) : tab_manager_(tab_manager) {}

Result<base::Value, std::string> TabsAPI::Invoke(
    const std::string& method,
    const base::Value::List& args) {
  if (method == "create") return Create(args);
  if (method == "query") return Query(args);
  if (method == "update") return Update(args);
  if (method == "remove") return Remove(args);
  if (method == "get") return Get(args);
  return Err("Unknown method: tabs." + method);
}

Result<base::Value, std::string> TabsAPI::Create(const base::Value::List& args) {
  GURL url;
  bool active = true;
  if (!args.empty() && args[0].is_dict()) {
    const auto& dict = args[0].GetDict();
    if (const std::string* url_str = dict.FindString("url")) {
      url = GURL(*url_str);
      if (!IsWebNavigableUrl(url))
        return Err("tabs.create: disallowed URL");
    }
    if (std::optional<bool> a = dict.FindBool("active"))
      active = *a;
  }
  auto result = tab_manager_->CreateTab(url);
  if (result.IsErr())
    return Err(result.UnwrapErr());
  auto info = tab_manager_->GetTabInfo(result.Value());
  if (info.IsOk())
    return Ok(base::Value(TabInfoToDict(info.Value())));
  return Err("Tab created but info unavailable");
}

Result<base::Value, std::string> TabsAPI::Query(const base::Value::List& args) {
  base::Value::List list;
  for (const auto& tab : tab_manager_->GetAllTabs()) {
    list.Append(TabInfoToDict(tab));
  }
  return Ok(base::Value(std::move(list)));
}

Result<base::Value, std::string> TabsAPI::Update(const base::Value::List& args) {
  if (args.size() < 2)
    return Err("tabs.update requires tabId and updateProperties");
  if (!args[0].is_int())
    return Err("tabId must be an integer");
  int64_t tab_id = args[0].GetInt();
  if (!args[1].is_dict())
    return Err("updateProperties must be an object");

  const auto& props = args[1].GetDict();
  TabId id(tab_id);

  if (const std::string* url = props.FindString("url")) {
    GURL target(*url);
    if (!IsWebNavigableUrl(target))
      return Err("tabs.update: disallowed URL");
    tab_manager_->NavigateTab(id, target);
  }
  if (std::optional<bool> active = props.FindBool("active")) {
    if (*active) tab_manager_->ActivateTab(id);
  }
  if (const std::string* title = props.FindString("title")) {
    tab_manager_->SetTabTitle(id, *title);
  }

  auto info = tab_manager_->GetTabInfo(id);
  if (info.IsOk())
    return Ok(base::Value(TabInfoToDict(info.Value())));
  return Err("Tab not found");
}

Result<base::Value, std::string> TabsAPI::Remove(const base::Value::List& args) {
  if (args.empty() || !args[0].is_int())
    return Err("tabs.remove requires tabId");
  tab_manager_->CloseTab(TabId(args[0].GetInt()));
  return Ok(base::Value());
}

Result<base::Value, std::string> TabsAPI::Get(const base::Value::List& args) {
  if (args.empty() || !args[0].is_int())
    return Err("tabs.get requires tabId");
  auto info = tab_manager_->GetTabInfo(TabId(args[0].GetInt()));
  if (info.IsOk())
    return Ok(base::Value(TabInfoToDict(info.Value())));
  return Err("Tab not found");
}

base::Value::Dict TabsAPI::TabInfoToDict(const TabInfo& info) {
  base::Value::Dict dict;
  dict.Set("id", static_cast<int>(info.id.value()));
  dict.Set("url", info.url.spec());
  dict.Set("title", info.title);
  dict.Set("active", info.id == tab_manager_->GetActiveTabId());
  dict.Set("pinned", info.pinned);
  dict.Set("audible", info.audible);
  dict.Set("discarded", info.discarded);
  return dict;
}

std::string TabsAPI::GetJSShim() const {
  return R"js(
(function() {
  const ns = 'tabs';
  chrome.tabs = {
    create: (props) => __veor_api_call(ns, 'create', [props || {}]),
    query: (queryInfo) => __veor_api_call(ns, 'query', [queryInfo || {}]),
    update: (tabId, props) => __veor_api_call(ns, 'update', [tabId, props]),
    remove: (tabId) => __veor_api_call(ns, 'remove', [tabId]),
    get: (tabId) => __veor_api_call(ns, 'get', [tabId]),
    onCreated: { addListener: () => {}, removeListener: () => {} },
    onUpdated: { addListener: () => {}, removeListener: () => {} },
    onRemoved: { addListener: () => {}, removeListener: () => {} },
  };
})();
)js";
}

}  // namespace veor
