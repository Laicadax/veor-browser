// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "url/gurl.h"

namespace veor {

class HashDatabase;

// ─────────────────────────────────────────────────────────────────────────────
// SafeBrowsingService
// ─────────────────────────────────────────────────────────────────────────────
// Integrates Google Safe Browsing API v4 for real-time URL threat detection.
// Falls back to a local HashDatabase for offline checks.
//
// Threat types checked:
//   MALWARE, SOCIAL_ENGINEERING, UNWANTED_SOFTWARE, POTENTIALLY_HARMFUL
//
// Architecture:
//   1. Local prefix match (4-byte SHA256) against HashDatabase
//   2. On cache miss → async HTTPS call to Safe Browsing API
//   3. Results cached with TTL to minimize API usage
// ─────────────────────────────────────────────────────────────────────────────

enum class ThreatType {
  kSafe,
  kMalware,
  kSocialEngineering,
  kUnwantedSoftware,
  kPotentiallyHarmful,
};

struct ThreatCheckResult {
  ThreatType threat_type = ThreatType::kSafe;
  std::string threat_url;
  base::Time check_time;
  bool from_cache = false;
};

class SafeBrowsingService {
 public:
  explicit SafeBrowsingService(const base::FilePath& profile_path);
  ~SafeBrowsingService();

  // Non-copyable, non-movable
  SafeBrowsingService(const SafeBrowsingService&) = delete;
  SafeBrowsingService& operator=(const SafeBrowsingService&) = delete;

  // Initialize local hash database. Must be called before any checks.
  bool Initialize();

  // Set Google Safe Browsing API key. If empty, only local checks are performed.
  void SetApiKey(const std::string& api_key);

  // Synchronous local check (fast, no network).
  // Returns true if URL is flagged by local database.
  // Thread-safe: acquires internal lock.
  bool IsUrlMaliciousLocal(const GURL& url);

  // Asynchronous full check (local + remote API).
  // Callback receives result. Safe to call from any thread.
  using CheckCallback = base::OnceCallback<void(const ThreatCheckResult&)>;
  void CheckUrl(const GURL& url, CheckCallback callback);

  // Convenience: synchronous wrapper that blocks for local result only.
  // Used by NetworkDelegateImpl for request interception.
  // Thread-safe: acquires internal lock.
  ThreatCheckResult CheckUrlSync(const GURL& url);

  // Force refresh of local hash database from remote.
  // Called periodically (every 30 min) or on demand.
  void UpdateHashDatabase();

  // Starts periodic background updates of the hash database.
  // interval: how often to check for updates (default 30 minutes).
  void StartPeriodicUpdates(base::TimeDelta interval = base::Minutes(30));

  // Stops periodic updates.
  void StopPeriodicUpdates();

  // Returns true if periodic updates are active.
  bool IsPeriodicUpdateActive() const;

  // Statistics (thread-safe)
  size_t GetCacheHitCount() const;
  size_t GetCacheMissCount() const;
  size_t GetApiCallCount() const { return api_calls_; }

 private:
  void OnApiResponse(const GURL& url, CheckCallback callback,
                     const std::string& response);
  void OnUpdateResponse(const std::string& response);
  void CacheResult(const GURL& url, const ThreatCheckResult& result);
  bool GetCachedResult(const GURL& url, ThreatCheckResult* out);

  std::string ComputeHashPrefix(const GURL& url);

  std::unique_ptr<HashDatabase> hash_db_;
  std::string api_key_;
  base::FilePath profile_path_;

  // Thread-safe LRU cache (URL spec → result)
  mutable base::Lock cache_lock_;
  std::unordered_map<std::string, ThreatCheckResult> cache_;
  size_t cache_hits_ = 0;
  size_t cache_misses_ = 0;
  size_t api_calls_ = 0;

  // Periodic update timer
  std::unique_ptr<base::RepeatingTimer> update_timer_;

  base::WeakPtrFactory<SafeBrowsingService> weak_factory_{this};
};

}  // namespace veor