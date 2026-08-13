// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace veor {

class SQLiteDatabase;

// ─────────────────────────────────────────────────────────────────────────────
// BookmarkEntry
// ─────────────────────────────────────────────────────────────────────────────

struct BookmarkEntry {
  int64_t id = 0;
  int64_t parent_id = 0;  // 0 = root
  std::string title;
  GURL url;  // Empty for folders
  base::Time date_added;
  bool is_folder = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// BookmarkDatabase
// ─────────────────────────────────────────────────────────────────────────────
// Persistent bookmark storage. Schema:
//   bookmarks(id, parent_id, title, url, date_added, is_folder)
// ─────────────────────────────────────────────────────────────────────────────

class BookmarkDatabase {
 public:
  BookmarkDatabase();
  ~BookmarkDatabase();

  // Open or create database at path
  bool Open(const base::FilePath& path);

  // Add
  int64_t AddBookmark(const GURL& url, const std::string& title,
                      int64_t parent_id = 0);
  int64_t AddFolder(const std::string& title, int64_t parent_id = 0);

  // Query
  std::vector<BookmarkEntry> GetChildren(int64_t parent_id);
  std::vector<BookmarkEntry> Search(const std::string& query);

  // Update / Delete
  void UpdateTitle(int64_t id, const std::string& title);
  void UpdateUrl(int64_t id, const GURL& url);
  void Delete(int64_t id);
  void Move(int64_t id, int64_t new_parent_id);

 private:
  void EnsureSchema();

  std::unique_ptr<SQLiteDatabase> db_;
};

}  // namespace veor
