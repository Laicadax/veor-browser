// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "extensions/ExtensionAPI.h"
#include "bookmarks/IBookmarkStore.h"

namespace veor {

class BookmarksAPI : public ExtensionAPI {
 public:
  explicit BookmarksAPI(IBookmarkStore* store);
  ~BookmarksAPI() override = default;

  const char* GetNamespace() const override { return "bookmarks"; }
  Result<base::Value, std::string> Invoke(
      const std::string& method,
      const base::Value::List& args) override;
  std::string GetJSShim() const override;

 private:
  Result<base::Value, std::string> Create(const base::Value::List& args);
  Result<base::Value, std::string> GetTree(const base::Value::List& args);
  Result<base::Value, std::string> Search(const base::Value::List& args);
  Result<base::Value, std::string> Remove(const base::Value::List& args);

  base::Value::Dict NodeToDict(const BookmarkNode& node);
  void AppendChildren(base::Value::List& list, BookmarkNodeId parent);

  IBookmarkStore* store_;
};

}  // namespace veor
