// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "bookmarks/IBookmarkStore.h"
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"

#include <memory>
#include "base/memory/raw_ptr.h"

namespace veor {

class IStorageEngine;

// ─────────────────────────────────────────────────────────────────────────────
// BookmarkStoreImpl
// ─────────────────────────────────────────────────────────────────────────────
// SQLite-backed hierarchical bookmarks.
//
// Schema:
//   bookmarks (id INTEGER PRIMARY KEY, parent_id INTEGER, type INTEGER,
//              title TEXT, url TEXT, sort_index INTEGER, date_added INTEGER)

class BookmarkStoreImpl : public IBookmarkStore {
 public:
  explicit BookmarkStoreImpl(IStorageEngine* storage);
  ~BookmarkStoreImpl() override = default;

  // IBookmarkStore
  Result<BookmarkNodeId, std::string> AddBookmark(
      const GURL& url,
      const std::string& title,
      BookmarkNodeId parent_folder) override;
  Result<BookmarkNodeId, std::string> AddFolder(
      const std::string& title,
      BookmarkNodeId parent_folder) override;
  Result<void, std::string> UpdateNode(BookmarkNodeId id,
                                        const std::string& new_title,
                                        const GURL& new_url) override;
  Result<void, std::string> DeleteNode(BookmarkNodeId id) override;
  Result<void, std::string> MoveNode(BookmarkNodeId id,
                                     BookmarkNodeId new_parent,
                                     int new_index) override;

  Result<BookmarkNode, std::string> GetNode(BookmarkNodeId id) const override;
  Result<std::vector<BookmarkNode>, std::string> GetChildren(
      BookmarkNodeId parent) const override;
  Result<std::vector<BookmarkNode>, std::string> Search(
      const std::string& query) const override;

  BookmarkNodeId GetRootId() const override;
  Result<std::vector<BookmarkNode>, std::string> GetTree() const override;

 private:
  Result<void, std::string> EnsureSchema();
  BookmarkNode RowToNode(class IStatement* stmt) const;

  raw_ptr<IStorageEngine> storage_;
  bool schema_initialized_ = false;
};

}  // namespace veor
