// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "history/IHistoryStore.h"
#include "base/time/time.h"
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"\n
#include <memory>

namespace veor {

class IStorageEngine;

// ─────────────────────────────────────────────────────────────────────────────
// HistoryStoreImpl
// ─────────────────────────────────────────────────────────────────────────────
// SQLite-backed history with FTS5 full-text search.
//
// Schema:
//   history (id INTEGER PRIMARY KEY, url TEXT, title TEXT, visit_time INTEGER,
//            visit_count INTEGER)
//   history_fts (virtual table using fts5(url, title, content=history))

class HistoryStoreImpl : public IHistoryStore {
 public:
  explicit HistoryStoreImpl(IStorageEngine* storage);
  ~HistoryStoreImpl() override = default;

  // IHistoryStore
  Result<void, std::string> AddVisit(const GURL& url,
                                    const std::string& title,
                                    base::Time visit_time) override;
  Result<HistoryQueryResult, std::string> Query(
      const HistoryQuery& query) override;
  Result<std::vector<HistoryEntry>, std::string> GetRecent(
      size_t count) override;
  Result<void, std::string> DeleteEntry(HistoryEntryId id) override;
  Result<void, std::string> DeleteRange(base::Time start,
                                         base::Time end) override;
  Result<void, std::string> DeleteAll() override;
  Result<size_t, std::string> ExpireOldEntries(base::Time cutoff) override;

 private:
  Result<void, std::string> EnsureSchema();

  raw_ptr<IStorageEngine> storage_;
  bool schema_initialized_ = false;
};

}  // namespace veor
