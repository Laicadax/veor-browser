// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/HistoryDatabase.h"
#include "base/files/file_path.h"
#include "url/gurl.h"

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "sql/statement.h"
#include "storage/SQLiteDatabase.h"

namespace veor {

namespace {

constexpr int kCurrentSchemaVersion = 1;

const char kCreateHistoryTable[] =
    "CREATE TABLE IF NOT EXISTS history ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  url TEXT NOT NULL,"
    "  title TEXT,"
    "  visit_time INTEGER NOT NULL,"
    "  visit_count INTEGER DEFAULT 1"
    ")";

const char kCreateUrlIndex[] =
    "CREATE INDEX IF NOT EXISTS idx_history_url ON history(url)";

const char kCreateTimeIndex[] =
    "CREATE INDEX IF NOT EXISTS idx_history_time ON history(visit_time)";

}  // namespace

HistoryDatabase::HistoryDatabase() = default;
HistoryDatabase::~HistoryDatabase() = default;

bool HistoryDatabase::Open(const base::FilePath& path) {
  db_ = std::make_unique<SQLiteDatabase>();
  if (!db_->Open(path))
    return false;
  EnsureSchema();
  return true;
}

void HistoryDatabase::EnsureSchema() {
  if (db_->GetSchemaVersion() < kCurrentSchemaVersion) {
    db_->Execute(kCreateHistoryTable);
    db_->Execute(kCreateUrlIndex);
    db_->Execute(kCreateTimeIndex);
    db_->SetSchemaVersion(kCurrentSchemaVersion);
  }
}

void HistoryDatabase::AddVisit(const GURL& url, const std::string& title) {
  if (!db_ || !db_->IsOpen())
    return;

  // Check if URL already exists
  sql::Statement check = db_->Prepare(
      "SELECT id, visit_count FROM history WHERE url = ?");
  check.BindString(0, url.spec());

  if (check.Step()) {
    int64_t id = check.ColumnInt64(0);
    int count = check.ColumnInt(1);

    sql::Statement update = db_->Prepare(
        "UPDATE history SET visit_count = ?, visit_time = ?, title = ? "
        "WHERE id = ?");
    update.BindInt(0, count + 1);
    update.BindInt64(1, base::Time::Now().ToInternalValue());
    update.BindString(2, title);
    update.BindInt64(3, id);
    update.Run();
  } else {
    sql::Statement insert = db_->Prepare(
        "INSERT INTO history (url, title, visit_time, visit_count) "
        "VALUES (?, ?, ?, 1)");
    insert.BindString(0, url.spec());
    insert.BindString(1, title);
    insert.BindInt64(2, base::Time::Now().ToInternalValue());
    insert.Run();
  }
}

std::vector<HistoryEntry> HistoryDatabase::QueryRecent(int limit) {
  std::vector<HistoryEntry> results;
  if (!db_ || !db_->IsOpen())
    return results;

  sql::Statement query = db_->Prepare(
      "SELECT id, url, title, visit_time, visit_count FROM history "
      "ORDER BY visit_time DESC LIMIT ?");
  query.BindInt(0, limit);

  while (query.Step()) {
    HistoryEntry entry;
    entry.id = query.ColumnInt64(0);
    entry.url = GURL(query.ColumnString(1));
    entry.title = query.ColumnString(2);
    entry.visit_time = base::Time::FromInternalValue(query.ColumnInt64(3));
    entry.visit_count = query.ColumnInt(4);
    results.push_back(entry);
  }
  return results;
}

std::vector<HistoryEntry> HistoryDatabase::Search(const std::string& query_str,
                                                   int limit) {
  std::vector<HistoryEntry> results;
  if (!db_ || !db_->IsOpen() || query_str.empty())
    return results;

  sql::Statement query = db_->Prepare(
      "SELECT id, url, title, visit_time, visit_count FROM history "
      "WHERE url LIKE ? OR title LIKE ? "
      "ORDER BY visit_time DESC LIMIT ?");
  std::string pattern = "%" + query_str + "%";
  query.BindString(0, pattern);
  query.BindString(1, pattern);
  query.BindInt(2, limit);

  while (query.Step()) {
    HistoryEntry entry;
    entry.id = query.ColumnInt64(0);
    entry.url = GURL(query.ColumnString(1));
    entry.title = query.ColumnString(2);
    entry.visit_time = base::Time::FromInternalValue(query.ColumnInt64(3));
    entry.visit_count = query.ColumnInt(4);
    results.push_back(entry);
  }
  return results;
}

void HistoryDatabase::DeleteEntry(int64_t id) {
  if (!db_ || !db_->IsOpen())
    return;
  sql::Statement del = db_->Prepare("DELETE FROM history WHERE id = ?");
  del.BindInt64(0, id);
  del.Run();
}

void HistoryDatabase::DeleteRange(base::Time start, base::Time end) {
  if (!db_ || !db_->IsOpen())
    return;
  sql::Statement del = db_->Prepare(
      "DELETE FROM history WHERE visit_time >= ? AND visit_time <= ?");
  del.BindInt64(0, start.ToInternalValue());
  del.BindInt64(1, end.ToInternalValue());
  del.Run();
}

void HistoryDatabase::ClearAll() {
  if (!db_ || !db_->IsOpen())
    return;
  db_->Execute("DELETE FROM history");
}

}  // namespace veor
