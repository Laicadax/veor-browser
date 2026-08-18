// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/api/BookmarksAPI.h"

#include "core/base/UrlSecurity.h"

namespace veor {

BookmarksAPI::BookmarksAPI(IBookmarkStore* store) : store_(store) {}

Result<base::Value, std::string> BookmarksAPI::Invoke(
    const std::string& method,
    const base::Value::List& args) {
  if (method == "create") return Create(args);
  if (method == "getTree") return GetTree(args);
  if (method == "search") return Search(args);
  if (method == "remove") return Remove(args);
  return Err("Unknown method: bookmarks." + method);
}

Result<base::Value, std::string> BookmarksAPI::Create(const base::Value::List& args) {
  if (args.empty() || !args[0].is_dict())
    return Err("bookmarks.create requires bookmark object");
  const auto& dict = args[0].GetDict();

  BookmarkNodeId parent = store_->GetRootId();
  if (const std::string* pid = dict.FindString("parentId")) {
    parent = BookmarkNodeId::FromString(*pid);
  }

  if (const std::string* url = dict.FindString("url")) {
    GURL target(*url);
    if (!target.is_valid() || IsDangerousNavigationScheme(target))
      return Err("bookmarks.create: disallowed URL");
    const std::string* title = dict.FindString("title");
    auto result = store_->AddBookmark(target, title ? *title : "", parent);
    if (result.IsOk()) {
      auto node = store_->GetNode(result.Value());
      if (node.IsOk()) return Ok(base::Value(NodeToDict(node.Value())));
    }
    return Err("Failed to create bookmark");
  }

  const std::string* title = dict.FindString("title");
  auto result = store_->AddFolder(title ? *title : "New Folder", parent);
  if (result.IsOk()) {
    auto node = store_->GetNode(result.Value());
    if (node.IsOk()) return Ok(base::Value(NodeToDict(node.Value())));
  }
  return Err("Failed to create folder");
}

Result<base::Value, std::string> BookmarksAPI::GetTree(const base::Value::List& args) {
  base::Value::List list;
  AppendChildren(list, store_->GetRootId());
  return Ok(base::Value(std::move(list)));
}

Result<base::Value, std::string> BookmarksAPI::Search(const base::Value::List& args) {
  if (args.empty()) return Err("bookmarks.search requires query");
  std::string query;
  if (args[0].is_string()) query = args[0].GetString();
  else if (args[0].is_dict()) {
    const auto& dict = args[0].GetDict();
    if (const std::string* q = dict.FindString("query")) query = *q;
  }
  auto result = store_->Search(query);
  if (result.IsErr()) return Err(result.UnwrapErr());
  base::Value::List list;
  for (const auto& node : result.Value()) {
    list.Append(NodeToDict(node));
  }
  return Ok(base::Value(std::move(list)));
}

Result<base::Value, std::string> BookmarksAPI::Remove(const base::Value::List& args) {
  if (args.empty() || !args[0].is_string())
    return Err("bookmarks.remove requires id string");
  auto id = BookmarkNodeId::FromString(args[0].GetString());
  auto result = store_->DeleteNode(id);
  if (result.IsOk()) return Ok(base::Value());
  return Err(result.UnwrapErr());
}

base::Value::Dict BookmarksAPI::NodeToDict(const BookmarkNode& node) {
  base::Value::Dict dict;
  dict.Set("id", node.id.ToString());
  dict.Set("title", node.title);
  if (node.is_folder) {
    dict.Set("children", base::Value(base::Value::Type::LIST));
  } else {
    dict.Set("url", node.url.spec());
  }
  dict.Set("dateAdded", static_cast<double>(node.date_added.ToJsTime()));
  return dict;
}

void BookmarksAPI::AppendChildren(base::Value::List& list, BookmarkNodeId parent) {
  auto result = store_->GetChildren(parent);
  if (result.IsErr()) return;
  for (const auto& node : result.Value()) {
    base::Value::Dict dict = NodeToDict(node);
    if (node.is_folder) {
      base::Value::List children;
      AppendChildren(children, node.id);
      dict.Set("children", base::Value(std::move(children)));
    }
    list.Append(std::move(dict));
  }
}

std::string BookmarksAPI::GetJSShim() const {
  return R"js(
(function() {
  const ns = 'bookmarks';
  chrome.bookmarks = {
    create: (bookmark) => __veor_api_call(ns, 'create', [bookmark]),
    getTree: () => __veor_api_call(ns, 'getTree', []),
    search: (query) => __veor_api_call(ns, 'search', [query]),
    remove: (id) => __veor_api_call(ns, 'remove', [id]),
    onCreated: { addListener: () => {}, removeListener: () => {} },
    onRemoved: { addListener: () => {}, removeListener: () => {} },
    onChanged: { addListener: () => {}, removeListener: () => {} },
  };
})();
)js";
}

}  // namespace veor
