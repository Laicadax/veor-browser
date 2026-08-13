// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "base/time/time.h"
#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// BookmarkNodeType
// ─────────────────────────────────────────────────────────────────────────────

enum class BookmarkNodeType {
  kFolder,
  kBookmark
};

// ─────────────────────────────────────────────────────────────────────────────
// BookmarkNode
// ─────────────────────────────────────────────────────────────────────────────

struct BookmarkNode {
  BookmarkNodeId id;
  BookmarkNodeId parent_id;
  BookmarkNodeType type;
  std::string title;
  GURL url;              // Empty for folders
  int sort_index = 0;
  base::Time date_added;
};

// ─────────────────────────────────────────────────────────────────────────────
// IBookmarkStore
// ─────────────────────────────────────────────────────────────────────────────
// Hierarchical bookmarks with tree structure.
//
// Thread safety: [IO Thread] for all methods.

class IBookmarkStore {
 public:
  virtual ~IBookmarkStore() = default;

  // ── CRUD ──
  // [IO Thread]
  virtual Result<BookmarkNodeId, std::string> AddBookmark(
      const GURL& url,
      const std::string& title,
      BookmarkNodeId parent_folder) = 0;

  // [IO Thread]
  virtual Result<BookmarkNodeId, std::string> AddFolder(
      const std::string& title,
      BookmarkNodeId parent_folder) = 0;

  // [IO Thread]
  virtual Result<void, std::string> UpdateNode(BookmarkNodeId id,
                                                const std::string& new_title,
                                                const GURL& new_url) = 0;

  // [IO Thread]
  virtual Result<void, std::string> DeleteNode(BookmarkNodeId id) = 0;

  // [IO Thread]
  virtual Result<void, std::string> MoveNode(BookmarkNodeId id,
                                             BookmarkNodeId new_parent,
                                             int new_index) = 0;

  // ── Queries ──
  // [IO Thread]
  virtual Result<BookmarkNode, std::string> GetNode(BookmarkNodeId id) const = 0;

  // [IO Thread]
  virtual Result<std::vector<BookmarkNode>, std::string> GetChildren(
      BookmarkNodeId parent) const = 0;

  // [IO Thread]
  virtual Result<std::vector<BookmarkNode>, std::string> Search(
      const std::string& query) const = 0;

  // ── Tree ──
  // Returns the root node (virtual, not stored).
  virtual BookmarkNodeId GetRootId() const = 0;

  // [IO Thread]
  virtual Result<std::vector<BookmarkNode>, std::string> GetTree() const = 0;
};

}  // namespace veor
