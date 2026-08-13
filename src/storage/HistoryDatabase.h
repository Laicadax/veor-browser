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
// HistoryEntry
// ─────────────────────────────────────────────────────────────────────────────

struct HistoryEntry {
  int64_t id = 0;
  GURL url;
  std::string title;
  base::Time visit_time;
  int visit_count = 1;
};

// ─────────────────────────────────────────────────────────────────────────────
// HistoryDatabase
// ─────────────────────────────────────────────────────────────────────────────
// Persistent history storage. Schema:
//   history(id, url, title, visit_time, visit_count)
// ─────────────────────────────────────────────────────────────────────────────

class HistoryDatabase {
 public:
  HistoryDatabase();
  ~HistoryDatabase();

  // Open or create database at path
  bool Open(const base::FilePath& path);

  // Add or update entry
  void AddVisit(const GURL& url, const std::string& title);

  // Query
  std::vector<HistoryEntry> QueryRecent(int limit);
  std::vector<HistoryEntry> Search(const std::string& query, int limit);

  // Delete
  void DeleteEntry(int64_t id);
  void DeleteRange(base::Time start, base::Time end);
  void ClearAll();

 private:
  void EnsureSchema();

  std::unique_ptr<SQLiteDatabase> db_;
};

}  // namespace veor
