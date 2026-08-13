// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "history/HistoryStoreImpl.h"

#include "base/time/time.h"
#include "core/logging/VeorLogger.h"
#include "infrastructure/storage/IStorageEngine.h"

namespace veor {

namespace {

constexpr int kHistorySchemaVersion = 1;

std::string TimeToSqlite(base::Time time) {
  return std::to_string(time.ToDeltaSinceWindowsEpoch().InSeconds());
}

base::Time TimeFromSqlite(int64_t seconds) {
  return base::Time::FromDeltaSinceWindowsEpoch(base::Seconds(seconds));
}

}  // namespace

HistoryStoreImpl::HistoryStoreImpl(IStorageEngine* storage)
    : storage_(storage) {}

Result<void, std::string> HistoryStoreImpl::EnsureSchema() {
  if (schema_initialized_)
    return Result<void, std::string>::Ok();

  auto version_result = storage_->Migrate(kHistorySchemaVersion);
  if (version_result.IsErr()) {
    auto err = version_result.UnwrapErr();
    return Result<void, std::string>::Err(err.ToString());
  }

  // Create history table
  auto create_result = storage_->Execute(
      "CREATE TABLE IF NOT EXISTS history ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  url TEXT NOT NULL,"
      "  title TEXT,"
      "  visit_time INTEGER NOT NULL,"
      "  visit_count INTEGER DEFAULT 1"
      ")");
  if (create_result.IsErr()) {
    return Result<void, std::string>::Err(
        create_result.UnwrapErr().ToString());
  }

  // Create FTS5 virtual table
  auto fts_result = storage_->Execute(
      "CREATE VIRTUAL TABLE IF NOT EXISTS history_fts USING fts5("
      "  url, title, content=history, content_rowid=id"
      ")");
  if (fts_result.IsErr()) {
    return Result<void, std::string>::Err(
        fts_result.UnwrapErr().ToString());
  }

  // Create indices
  storage_->Execute(
      "CREATE INDEX IF NOT EXISTS idx_history_time ON history(visit_time)");
  storage_->Execute(
      "CREATE INDEX IF NOT EXISTS idx_history_url ON history(url)");

  schema_initialized_ = true;
  return Result<void, std::string>::Ok();
}

Result<void, std::string> HistoryStoreImpl::AddVisit(const GURL& url,
                                                     const std::string& title,
                                                     base::Time visit_time) {
  auto schema_result = EnsureSchema();
  if (schema_result.IsErr())
    return schema_result;

  // Check if URL exists within the same day
  base::Time day_start = visit_time - base::Time::FromDeltaSinceWindowsEpoch(
      base::Seconds(visit_time.ToDeltaSinceWindowsEpoch().InSeconds() % 86400));
  base::Time day_end = day_start + base::Days(1);

  auto check_stmt = storage_->Prepare(
      "SELECT id, visit_count FROM history WHERE url = ? AND visit_time >= ? AND visit_time < ?");
  if (check_stmt.IsErr()) {
    return Result<void, std::string>::Err(
        check_stmt.UnwrapErr().ToString());
  }

  auto stmt = std::move(check_stmt).Unwrap();
  stmt->BindString(1, url.spec());
  stmt->BindInt64(2, day_start.ToDeltaSinceWindowsEpoch().InSeconds());
  stmt->BindInt64(3, day_end.ToDeltaSinceWindowsEpoch().InSeconds());

  auto step = stmt->Step();
  if (step.IsErr()) {
    return Result<void, std::string>::Err(step.UnwrapErr().ToString());
  }

  if (step.Unwrap()) {
    // Update existing entry
    int64_t id = stmt->GetInt64(0);
    int count = stmt->GetInt(1) + 1;

    auto update = storage_->Execute(
        "UPDATE history SET visit_count = ?, title = ?, visit_time = ? WHERE id = ?",
        {std::to_string(count), title, TimeToSqlite(visit_time), std::to_string(id)});
    if (update.IsErr()) {
      return Result<void, std::string>::Err(update.UnwrapErr().ToString());
    }
  } else {
    // Insert new entry
    auto insert = storage_->Execute(
        "INSERT INTO history (url, title, visit_time) VALUES (?, ?, ?)",
        {url.spec(), title, TimeToSqlite(visit_time)});
    if (insert.IsErr()) {
      return Result<void, std::string>::Err(insert.UnwrapErr().ToString());
    }
  }

  return Result<void, std::string>::Ok();
}

Result<HistoryQueryResult, std::string> HistoryStoreImpl::Query(
    const HistoryQuery& query) {
  auto schema_result = EnsureSchema();
  if (schema_result.IsErr()) {
    return Result<HistoryQueryResult, std::string>::Err(
        schema_result.UnwrapErr());
  }

  HistoryQueryResult result;

  std::string sql;
  std::vector<std::string> params;

  if (query.text_query.empty()) {
    sql = "SELECT id, url, title, visit_time, visit_count FROM history"
          " WHERE visit_time >= ? AND visit_time <= ?"
          " ORDER BY visit_time " + std::string(query.descending ? "DESC" : "ASC") +
          " LIMIT ? OFFSET ?";
    params = {
        TimeToSqlite(query.start_time),
        TimeToSqlite(query.end_time),
        std::to_string(query.max_results),
        std::to_string(query.offset)};
  } else {
    sql = "SELECT h.id, h.url, h.title, h.visit_time, h.visit_count"
          " FROM history h"
          " JOIN history_fts fts ON h.id = fts.rowid"
          " WHERE history_fts MATCH ?"
          " AND h.visit_time >= ? AND h.visit_time <= ?"
          " ORDER BY h.visit_time " + std::string(query.descending ? "DESC" : "ASC") +
          " LIMIT ? OFFSET ?";
    params = {
        query.text_query,
        TimeToSqlite(query.start_time),
        TimeToSqlite(query.end_time),
        std::to_string(query.max_results),
        std::to_string(query.offset)};
  }

  auto stmt_result = storage_->Prepare(sql);
  if (stmt_result.IsErr()) {
    return Result<HistoryQueryResult, std::string>::Err(
        stmt_result.UnwrapErr().ToString());
  }

  auto stmt = std::move(stmt_result).Unwrap();
  for (size_t i = 0; i < params.size(); ++i) {
    stmt->BindString(static_cast<int>(i + 1), params[i]);
  }

  while (true) {
    auto step = stmt->Step();
    if (step.IsErr()) {
      return Result<HistoryQueryResult, std::string>::Err(
          step.UnwrapErr().ToString());
    }
    if (!step.Unwrap())
      break;

    HistoryEntry entry;
    entry.id = HistoryEntryId(static_cast<uint64_t>(stmt->GetInt64(0)));
    entry.url = GURL(stmt->GetString(1));
    entry.title = stmt->GetString(2);
    entry.visit_time = TimeFromSqlite(stmt->GetInt64(3));
    entry.visit_count = stmt->GetInt(4);
    result.entries.push_back(entry);
  }

  // Count total
  auto count_stmt = storage_->Prepare(
      "SELECT COUNT(*) FROM history WHERE visit_time >= ? AND visit_time <= ?");
  if (count_stmt.IsOk()) {
    auto cs = std::move(count_stmt).Unwrap();
    cs->BindString(1, TimeToSqlite(query.start_time));
    cs->BindString(2, TimeToSqlite(query.end_time));
    if (cs->Step().Unwrap()) {
      result.total_count = static_cast<size_t>(cs->GetInt64(0));
    }
  }

  return Result<HistoryQueryResult, std::string>::Ok(std::move(result));
}

Result<std::vector<HistoryEntry>, std::string> HistoryStoreImpl::GetRecent(
    size_t count) {
  HistoryQuery query;
  query.start_time = base::Time::Min();
  query.end_time = base::Time::Max();
  query.max_results = count;
  query.descending = true;

  auto result = Query(query);
  if (result.IsErr()) {
    return Result<std::vector<HistoryEntry>, std::string>::Err(
        result.UnwrapErr());
  }
  return Result<std::vector<HistoryEntry>, std::string>::Ok(
      std::move(result.Unwrap().entries));
}

Result<void, std::string> HistoryStoreImpl::DeleteEntry(HistoryEntryId id) {
  auto schema_result = EnsureSchema();
  if (schema_result.IsErr())
    return schema_result;

  auto result = storage_->Execute(
      "DELETE FROM history WHERE id = ?",
      {std::to_string(id.value())});
  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }
  return Result<void, std::string>::Ok();
}

Result<void, std::string> HistoryStoreImpl::DeleteRange(base::Time start,
                                                           base::Time end) {
  auto schema_result = EnsureSchema();
  if (schema_result.IsErr())
    return schema_result;

  auto result = storage_->Execute(
      "DELETE FROM history WHERE visit_time >= ? AND visit_time <= ?",
      {TimeToSqlite(start), TimeToSqlite(end)});
  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }
  return Result<void, std::string>::Ok();
}

Result<void, std::string> HistoryStoreImpl::DeleteAll() {
  auto schema_result = EnsureSchema();
  if (schema_result.IsErr())
    return schema_result;

  auto result = storage_->Execute("DELETE FROM history");
  if (result.IsErr()) {
    return Result<void, std::string>::Err(result.UnwrapErr().ToString());
  }
  return Result<void, std::string>::Ok();
}

Result<size_t, std::string> HistoryStoreImpl::ExpireOldEntries(
    base::Time cutoff) {
  auto schema_result = EnsureSchema();
  if (schema_result.IsErr()) {
    return Result<size_t, std::string>::Err(schema_result.UnwrapErr());
  }

  auto count_stmt = storage_->Prepare(
      "SELECT COUNT(*) FROM history WHERE visit_time < ?");
  size_t count = 0;
  if (count_stmt.IsOk()) {
    auto cs = std::move(count_stmt).Unwrap();
    cs->BindString(1, TimeToSqlite(cutoff));
    if (cs->Step().Unwrap()) {
      count = static_cast<size_t>(cs->GetInt64(0));
    }
  }

  auto result = storage_->Execute(
      "DELETE FROM history WHERE visit_time < ?",
      {TimeToSqlite(cutoff)});
  if (result.IsErr()) {
    return Result<size_t, std::string>::Err(result.UnwrapErr().ToString());
  }

  VEOR_LOGI(LogCategory::kHistory,
            "Expired " + std::to_string(count) + " old history entries");
  return Result<size_t, std::string>::Ok(count);
}

}  // namespace veor
