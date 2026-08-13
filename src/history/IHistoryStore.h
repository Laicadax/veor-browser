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
// HistoryEntry
// ─────────────────────────────────────────────────────────────────────────────

struct HistoryEntry {
  HistoryEntryId id;
  GURL url;
  std::string title;
  base::Time visit_time;
  int visit_count = 1;
};

// ─────────────────────────────────────────────────────────────────────────────
// HistoryQuery
// ─────────────────────────────────────────────────────────────────────────────

struct HistoryQuery {
  std::string text_query;           // FTS search text
  base::Time start_time;            // Inclusive
  base::Time end_time;              // Inclusive
  size_t max_results = 100;
  size_t offset = 0;
  bool descending = true;           // Newest first
};

struct HistoryQueryResult {
  std::vector<HistoryEntry> entries;
  size_t total_count = 0;           // For pagination
};

// ─────────────────────────────────────────────────────────────────────────────
// IHistoryStore
// ─────────────────────────────────────────────────────────────────────────────
// Full-text searchable history with expiration.
//
// Thread safety: [IO Thread] for all methods.

class IHistoryStore {
 public:
  virtual ~IHistoryStore() = default;

  // ── Writes ──
  // Adds or updates a history entry. Increments visit_count if URL exists
  // within the same day.
  // [IO Thread]
  virtual Result<void, std::string> AddVisit(const GURL& url,
                                            const std::string& title,
                                            base::Time visit_time) = 0;

  // ── Reads ──
  // Full-text search with filtering.
  // [IO Thread]
  virtual Result<HistoryQueryResult, std::string> Query(
      const HistoryQuery& query) = 0;

  // Get recent visits (convenience method).
  // [IO Thread]
  virtual Result<std::vector<HistoryEntry>, std::string> GetRecent(
      size_t count) = 0;

  // ── Deletion ──
  // [IO Thread]
  virtual Result<void, std::string> DeleteEntry(HistoryEntryId id) = 0;
  virtual Result<void, std::string> DeleteRange(base::Time start,
                                                 base::Time end) = 0;
  virtual Result<void, std::string> DeleteAll() = 0;

  // ── Maintenance ──
  // Expires entries older than the cutoff.
  // [IO Thread]
  virtual Result<size_t, std::string> ExpireOldEntries(base::Time cutoff) = 0;
};

}  // namespace veor
