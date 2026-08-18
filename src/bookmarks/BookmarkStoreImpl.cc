// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bookmarks/BookmarkStoreImpl.h"
#include "url/gurl.h"

#include "core/logging/VeorLogger.h"
#include "infrastructure/storage/IStorageEngine.h"

namespace veor {

namespace {

constexpr int kBookmarkSchemaVersion = 1;
constexpr BookmarkNodeId kRootId(1);

}  // namespace

BookmarkStoreImpl::BookmarkStoreImpl(IStorageEngine* storage)
    : storage_(storage) {}

Result<void, std::string> BookmarkStoreImpl::EnsureSchema() {
  if (schema_initialized_)
    return Result<void, std::string>::Ok();

  auto v = storage_->Migrate(kBookmarkSchemaVersion);
  if (v.IsErr())
    return Result<void, std::string>::Err(v.UnwrapErr().ToString());

  storage_->Execute(
      "CREATE TABLE IF NOT EXISTS bookmarks ("
      "id INTEGER PRIMARY KEY,"
      "parent_id INTEGER DEFAULT 1,"
      "type INTEGER,"
      "title TEXT,"
      "url TEXT,"
      "sort_index INTEGER,"
      "date_added INTEGER"
      ")");
  storage_->Execute(
      "CREATE INDEX IF NOT EXISTS idx_bookmarks_parent ON bookmarks(parent_id)");

  schema_initialized_ = true;
  return Result<void, std::string>::Ok();
}

BookmarkNode BookmarkStoreImpl::RowToNode(IStatement* stmt) const {
  BookmarkNode node;
  node.id = BookmarkNodeId(static_cast<uint64_t>(stmt->GetInt64(0)));
  node.parent_id = BookmarkNodeId(static_cast<uint64_t>(stmt->GetInt64(1)));
  node.type = stmt->GetInt(2) == 0 ? BookmarkNodeType::kFolder
                                    : BookmarkNodeType::kBookmark;
  node.title = stmt->GetString(3);
  node.url = GURL(stmt->GetString(4));
  node.sort_index = stmt->GetInt(5);
  return node;
}

Result<BookmarkNodeId, std::string> BookmarkStoreImpl::AddBookmark(
    const GURL& url,
    const std::string& title,
    BookmarkNodeId parent_folder) {
  EnsureSchema();
  auto r = storage_->Execute(
      "INSERT INTO bookmarks (parent_id, type, title, url, sort_index, date_added)"
      " VALUES (?, 1, ?, ?, 0, strftime('%s','now'))",
      {std::to_string(parent_folder.Unwrap()), title, url.spec()});
  if (r.IsErr())
    return Result<BookmarkNodeId, std::string>::Err(r.UnwrapErr().ToString());

  auto stmt = storage_->Prepare("SELECT last_insert_rowid()")
                  .Unwrap();
  stmt->Step();
  int64_t rowid = stmt->GetInt64(0);
  return Result<BookmarkNodeId, std::string>::Ok(
      BookmarkNodeId(static_cast<uint64_t>(rowid)));
}

Result<BookmarkNodeId, std::string> BookmarkStoreImpl::AddFolder(
    const std::string& title,
    BookmarkNodeId parent_folder) {
  EnsureSchema();
  auto r = storage_->Execute(
      "INSERT INTO bookmarks (parent_id, type, title, sort_index, date_added)"
      " VALUES (?, 0, ?, 0, strftime('%s','now'))",
      {std::to_string(parent_folder.Unwrap()), title});
  if (r.IsErr())
    return Result<BookmarkNodeId, std::string>::Err(r.UnwrapErr().ToString());

  auto stmt = storage_->Prepare("SELECT last_insert_rowid()")
                  .Unwrap();
  stmt->Step();
  int64_t rowid = stmt->GetInt64(0);
  return Result<BookmarkNodeId, std::string>::Ok(
      BookmarkNodeId(static_cast<uint64_t>(rowid)));
}

Result<void, std::string> BookmarkStoreImpl::UpdateNode(
    BookmarkNodeId id,
    const std::string& new_title,
    const GURL& new_url) {
  EnsureSchema();
  auto r = storage_->Execute(
      "UPDATE bookmarks SET title = ?, url = ? WHERE id = ?",
      {new_title, new_url.spec(), std::to_string(id.Unwrap())});
  if (r.IsErr())
    return Result<void, std::string>::Err(r.UnwrapErr().ToString());
  return Result<void, std::string>::Ok();
}

Result<void, std::string> BookmarkStoreImpl::DeleteNode(BookmarkNodeId id) {
  EnsureSchema();
  // Recursive delete: first delete children
  storage_->Execute("DELETE FROM bookmarks WHERE parent_id = ?",
                    {std::to_string(id.Unwrap())});
  auto r = storage_->Execute("DELETE FROM bookmarks WHERE id = ?",
                             {std::to_string(id.Unwrap())});
  if (r.IsErr())
    return Result<void, std::string>::Err(r.UnwrapErr().ToString());
  return Result<void, std::string>::Ok();
}

Result<void, std::string> BookmarkStoreImpl::MoveNode(
    BookmarkNodeId id,
    BookmarkNodeId new_parent,
    int new_index) {
  EnsureSchema();
  auto r = storage_->Execute(
      "UPDATE bookmarks SET parent_id = ?, sort_index = ? WHERE id = ?",
      {std::to_string(new_parent.Unwrap()), std::to_string(new_index),
       std::to_string(id.Unwrap())});
  if (r.IsErr())
    return Result<void, std::string>::Err(r.UnwrapErr().ToString());
  return Result<void, std::string>::Ok();
}

Result<BookmarkNode, std::string> BookmarkStoreImpl::GetNode(
    BookmarkNodeId id) const {
  const_cast<BookmarkStoreImpl*>(this)->EnsureSchema();
  auto stmt_result = storage_->Prepare(
      "SELECT id, parent_id, type, title, url, sort_index FROM bookmarks WHERE id = ?");
  if (stmt_result.IsErr())
    return Result<BookmarkNode, std::string>::Err(
        stmt_result.UnwrapErr().ToString());

  auto stmt = std::move(stmt_result).Unwrap();
  stmt->BindString(1, std::to_string(id.Unwrap()));

  auto step = stmt->Step();
  if (step.IsErr())
    return Result<BookmarkNode, std::string>::Err(step.UnwrapErr().ToString());
  if (!step.Unwrap())
    return Result<BookmarkNode, std::string>::Err("Bookmark not found");

  return Result<BookmarkNode, std::string>::Ok(RowToNode(stmt.get()));
}

Result<std::vector<BookmarkNode>, std::string> BookmarkStoreImpl::GetChildren(
    BookmarkNodeId parent) const {
  const_cast<BookmarkStoreImpl*>(this)->EnsureSchema();
  std::vector<BookmarkNode> result;

  auto stmt_result = storage_->Prepare(
      "SELECT id, parent_id, type, title, url, sort_index FROM bookmarks"
      " WHERE parent_id = ? ORDER BY sort_index");
  if (stmt_result.IsErr())
    return Result<std::vector<BookmarkNode>, std::string>::Err(
        stmt_result.UnwrapErr().ToString());

  auto stmt = std::move(stmt_result).Unwrap();
  stmt->BindString(1, std::to_string(parent.Unwrap()));

  while (true) {
    auto step = stmt->Step();
    if (step.IsErr())
      return Result<std::vector<BookmarkNode>, std::string>::Err(
          step.UnwrapErr().ToString());
    if (!step.Unwrap())
      break;
    result.push_back(RowToNode(stmt.get()));
  }

  return Result<std::vector<BookmarkNode>, std::string>::Ok(std::move(result));
}

Result<std::vector<BookmarkNode>, std::string> BookmarkStoreImpl::Search(
    const std::string& query) const {
  const_cast<BookmarkStoreImpl*>(this)->EnsureSchema();
  std::vector<BookmarkNode> result;

  auto stmt_result = storage_->Prepare(
      "SELECT id, parent_id, type, title, url, sort_index FROM bookmarks"
      " WHERE title LIKE ? OR url LIKE ?");
  if (stmt_result.IsErr())
    return Result<std::vector<BookmarkNode>, std::string>::Err(
        stmt_result.UnwrapErr().ToString());

  auto stmt = std::move(stmt_result).Unwrap();
  stmt->BindString(1, "%" + query + "%");
  stmt->BindString(2, "%" + query + "%");

  while (true) {
    auto step = stmt->Step();
    if (step.IsErr())
      return Result<std::vector<BookmarkNode>, std::string>::Err(
          step.UnwrapErr().ToString());
    if (!step.Unwrap())
      break;
    result.push_back(RowToNode(stmt.get()));
  }

  return Result<std::vector<BookmarkNode>, std::string>::Ok(std::move(result));
}

BookmarkNodeId BookmarkStoreImpl::GetRootId() const {
  return kRootId;
}

Result<std::vector<BookmarkNode>, std::string> BookmarkStoreImpl::GetTree() const {
  return GetChildren(kRootId);
}

}  // namespace veor
