// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/BookmarkDatabase.h"

#include "base/time/time.h"
#include "sql/statement.h"
#include "storage/SQLiteDatabase.h"

namespace veor {

namespace {

constexpr int kCurrentSchemaVersion = 1;

const char kCreateBookmarksTable[] =
    "CREATE TABLE IF NOT EXISTS bookmarks ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  parent_id INTEGER DEFAULT 0,"
    "  title TEXT NOT NULL,"
    "  url TEXT,"
    "  date_added INTEGER NOT NULL,"
    "  is_folder INTEGER DEFAULT 0"
    ")";

const char kCreateParentIndex[] =
    "CREATE INDEX IF NOT EXISTS idx_bookmarks_parent ON bookmarks(parent_id)";

const char kCreateUrlIndex[] =
    "CREATE INDEX IF NOT EXISTS idx_bookmarks_url ON bookmarks(url)";

}  // namespace

BookmarkDatabase::BookmarkDatabase() = default;
BookmarkDatabase::~BookmarkDatabase() = default;

bool BookmarkDatabase::Open(const base::FilePath& path) {
  db_ = std::make_unique<SQLiteDatabase>();
  if (!db_->Open(path))
    return false;
  EnsureSchema();
  return true;
}

void BookmarkDatabase::EnsureSchema() {
  if (db_->GetSchemaVersion() < kCurrentSchemaVersion) {
    db_->Execute(kCreateBookmarksTable);
    db_->Execute(kCreateParentIndex);
    db_->Execute(kCreateUrlIndex);
    db_->SetSchemaVersion(kCurrentSchemaVersion);
  }
}

int64_t BookmarkDatabase::AddBookmark(const GURL& url,
                                      const std::string& title,
                                      int64_t parent_id) {
  if (!db_ || !db_->IsOpen() || !url.is_valid())
    return 0;

  sql::Statement insert = db_->Prepare(
      "INSERT INTO bookmarks (parent_id, title, url, date_added, is_folder) "
      "VALUES (?, ?, ?, ?, 0)");
  insert.BindInt64(0, parent_id);
  insert.BindString(1, title);
  insert.BindString(2, url.spec());
  insert.BindInt64(3, base::Time::Now().ToInternalValue());

  if (!insert.Run())
    return 0;
  return db_->db()->GetLastInsertRowId();
}

int64_t BookmarkDatabase::AddFolder(const std::string& title,
                                    int64_t parent_id) {
  if (!db_ || !db_->IsOpen())
    return 0;

  sql::Statement insert = db_->Prepare(
      "INSERT INTO bookmarks (parent_id, title, url, date_added, is_folder) "
      "VALUES (?, ?, NULL, ?, 1)");
  insert.BindInt64(0, parent_id);
  insert.BindString(1, title);
  insert.BindInt64(2, base::Time::Now().ToInternalValue());

  if (!insert.Run())
    return 0;
  return db_->db()->GetLastInsertRowId();
}

std::vector<BookmarkEntry> BookmarkDatabase::GetChildren(int64_t parent_id) {
  std::vector<BookmarkEntry> results;
  if (!db_ || !db_->IsOpen())
    return results;

  sql::Statement query = db_->Prepare(
      "SELECT id, parent_id, title, url, date_added, is_folder "
      "FROM bookmarks WHERE parent_id = ? ORDER BY is_folder DESC, title");
  query.BindInt64(0, parent_id);

  while (query.Step()) {
    BookmarkEntry entry;
    entry.id = query.ColumnInt64(0);
    entry.parent_id = query.ColumnInt64(1);
    entry.title = query.ColumnString(2);
    std::string url_str = query.ColumnString(3);
    if (!url_str.empty())
      entry.url = GURL(url_str);
    entry.date_added = base::Time::FromInternalValue(query.ColumnInt64(4));
    entry.is_folder = query.ColumnBool(5);
    results.push_back(entry);
  }
  return results;
}

std::vector<BookmarkEntry> BookmarkDatabase::Search(const std::string& query) {
  std::vector<BookmarkEntry> results;
  if (!db_ || !db_->IsOpen() || query.empty())
    return results;

  sql::Statement stmt = db_->Prepare(
      "SELECT id, parent_id, title, url, date_added, is_folder "
      "FROM bookmarks WHERE title LIKE ? OR url LIKE ? "
      "ORDER BY title");
  std::string pattern = "%" + query + "%";
  stmt.BindString(0, pattern);
  stmt.BindString(1, pattern);

  while (stmt.Step()) {
    BookmarkEntry entry;
    entry.id = stmt.ColumnInt64(0);
    entry.parent_id = stmt.ColumnInt64(1);
    entry.title = stmt.ColumnString(2);
    std::string url_str = stmt.ColumnString(3);
    if (!url_str.empty())
      entry.url = GURL(url_str);
    entry.date_added = base::Time::FromInternalValue(stmt.ColumnInt64(4));
    entry.is_folder = stmt.ColumnBool(5);
    results.push_back(entry);
  }
  return results;
}

void BookmarkDatabase::UpdateTitle(int64_t id, const std::string& title) {
  if (!db_ || !db_->IsOpen())
    return;
  sql::Statement update = db_->Prepare(
      "UPDATE bookmarks SET title = ? WHERE id = ?");
  update.BindString(0, title);
  update.BindInt64(1, id);
  update.Run();
}

void BookmarkDatabase::UpdateUrl(int64_t id, const GURL& url) {
  if (!db_ || !db_->IsOpen() || !url.is_valid())
    return;
  sql::Statement update = db_->Prepare(
      "UPDATE bookmarks SET url = ? WHERE id = ?");
  update.BindString(0, url.spec());
  update.BindInt64(1, id);
  update.Run();
}

void BookmarkDatabase::Delete(int64_t id) {
  if (!db_ || !db_->IsOpen())
    return;

  // Delete children first
  sql::Statement del_children = db_->Prepare(
      "DELETE FROM bookmarks WHERE parent_id = ?");
  del_children.BindInt64(0, id);
  del_children.Run();

  // Delete self
  sql::Statement del = db_->Prepare("DELETE FROM bookmarks WHERE id = ?");
  del.BindInt64(0, id);
  del.Run();
}

void BookmarkDatabase::Move(int64_t id, int64_t new_parent_id) {
  if (!db_ || !db_->IsOpen())
    return;
  sql::Statement update = db_->Prepare(
      "UPDATE bookmarks SET parent_id = ? WHERE id = ?");
  update.BindInt64(0, new_parent_id);
  update.BindInt64(1, id);
  update.Run();
}

}  // namespace veor
