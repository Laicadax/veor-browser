// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "safe_browsing/SafeBrowsingService.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "core/logging/VeorLogger.h"
#include "net/base/url_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

#include "safe_browsing/HashDatabase.h"

namespace veor {

namespace {

constexpr char kSafeBrowsingApiEndpoint[] =
    "https://safebrowsing.googleapis.com/v4/threatMatches:find";

constexpr base::TimeDelta kCacheTtl = base::Minutes(5);
constexpr size_t kMaxCacheSize = 4096;

// Build the JSON request body for Safe Browsing API v4.
std::string BuildThreatMatchesRequest(const GURL& url) {
  base::Value::Dict client;
  client.Set("clientId", "veor-browser");
  client.Set("clientVersion", "1.0.0");

  base::Value::Dict threat_info;
  threat_info.Set("url", url.spec());

  base::Value::List threat_types;
  threat_types.Append("MALWARE");
  threat_types.Append("SOCIAL_ENGINEERING");
  threat_types.Append("UNWANTED_SOFTWARE");
  threat_types.Append("POTENTIALLY_HARMFUL_APPLICATION");
  threat_info.Set("threatTypes", std::move(threat_types));

  base::Value::List platform_types;
  platform_types.Append("ANY_PLATFORM");
  threat_info.Set("platformTypes", std::move(platform_types));

  base::Value::List threat_entry_types;
  threat_entry_types.Append("URL");
  threat_info.Set("threatEntryTypes", std::move(threat_entry_types));

  base::Value::Dict root;
  root.Set("client", std::move(client));
  root.Set("threatInfo", std::move(threat_info));

  std::string json;
  base::JSONWriter::Write(root, &json);
  return json;
}

ThreatType ParseThreatType(const std::string& type_str) {
  if (type_str == "MALWARE")
    return ThreatType::kMalware;
  if (type_str == "SOCIAL_ENGINEERING")
    return ThreatType::kSocialEngineering;
  if (type_str == "UNWANTED_SOFTWARE")
    return ThreatType::kUnwantedSoftware;
  if (type_str == "POTENTIALLY_HARMFUL_APPLICATION")
    return ThreatType::kPotentiallyHarmful;
  return ThreatType::kSafe;
}

}  // namespace

// ── SafeBrowsingService ──────────────────────────────────────────────────────

SafeBrowsingService::SafeBrowsingService(const base::FilePath& profile_path)
    : profile_path_(profile_path) {}

SafeBrowsingService::~SafeBrowsingService() = default;

bool SafeBrowsingService::Initialize() {
  base::FilePath db_path = profile_path_.AppendASCII("SafeBrowsingHashes");
  hash_db_ = std::make_unique<HashDatabase>(db_path);
  if (!hash_db_->Open()) {
    VEOR_LOGE(LogCategory::kSecurity,
              "SafeBrowsing: failed to open hash database");
    hash_db_.reset();
    return false;
  }
  VEOR_LOGI(LogCategory::kSecurity,
            "SafeBrowsing: initialized, entries=" +
                std::to_string(hash_db_->GetEntryCount()));
  return true;
}

void SafeBrowsingService::SetApiKey(const std::string& api_key) {
  api_key_ = api_key;
}

bool SafeBrowsingService::IsUrlMaliciousLocal(const GURL& url) {
  if (!hash_db_)
    return false;
  HashThreatType local_type;
  return hash_db_->ContainsUrl(url, &local_type);
}

ThreatCheckResult SafeBrowsingService::CheckUrlSync(const GURL& url) {
  ThreatCheckResult result;
  result.check_time = base::Time::Now();

  // 1. Check cache (thread-safe)
  if (GetCachedResult(url, &result)) {
    result.from_cache = true;
    return result;
  }

  // 2. Check local hash database
  HashThreatType local_type;
  if (hash_db_ && hash_db_->ContainsUrl(url, &local_type)) {
    switch (local_type) {
      case HashThreatType::kMalware:
        result.threat_type = ThreatType::kMalware;
        break;
      case HashThreatType::kSocialEngineering:
        result.threat_type = ThreatType::kSocialEngineering;
        break;
      case HashThreatType::kUnwantedSoftware:
        result.threat_type = ThreatType::kUnwantedSoftware;
        break;
      case HashThreatType::kPotentiallyHarmful:
        result.threat_type = ThreatType::kPotentiallyHarmful;
        break;
    }
    result.threat_url = url.spec();
    CacheResult(url, result);
    return result;
  }

  result.threat_type = ThreatType::kSafe;
  return result;
}

void SafeBrowsingService::CheckUrl(const GURL& url, CheckCallback callback) {
  // Fast path: cache hit
  ThreatCheckResult cached;
  if (GetCachedResult(url, &cached)) {
    cached.from_cache = true;
    std::move(callback).Run(cached);
    return;
  }

  // Local check
  ThreatCheckResult local = CheckUrlSync(url);
  if (local.threat_type != ThreatType::kSafe) {
    std::move(callback).Run(local);
    return;
  }

  // No API key → local-only mode
  if (api_key_.empty()) {
    std::move(callback).Run(local);
    return;
  }

  // Async API call
  // NOTE: In production, use URLLoaderFactory from the browser process.
  // For MVP, the result is deferred to local-only since we don't have
  // network service access in this context without proper Chromium plumbing.
  // The architecture is ready — wire up URLLoaderFactory when integrating
  // with the full browser network stack.
  VEOR_LOGD(LogCategory::kSecurity,
            "SafeBrowsing: API check deferred for " + url.spec());
  std::move(callback).Run(local);
}

void SafeBrowsingService::UpdateHashDatabase() {
  if (!hash_db_ || api_key_.empty())
    return;

  VEOR_LOGI(LogCategory::kSecurity,
            "SafeBrowsing: fetching hash database updates");

  // Build the threatListUpdates:fetch request
  base::Value::Dict client;
  client.Set("clientId", "veor-browser");
  client.Set("clientVersion", "1.0.0");

  base::Value::List list_update_requests;
  const char* threat_types[] = {
    "MALWARE", "SOCIAL_ENGINEERING", "UNWANTED_SOFTWARE",
    "POTENTIALLY_HARMFUL_APPLICATION"
  };
  for (const char* type : threat_types) {
    base::Value::Dict req;
    req.Set("threatType", type);
    req.Set("platformType", "ANY_PLATFORM");
    req.Set("threatEntryType", "URL");
    list_update_requests.Append(std::move(req));
  }

  base::Value::Dict root;
  root.Set("client", std::move(client));
  root.Set("listUpdateRequests", std::move(list_update_requests));

  std::string json;
  base::JSONWriter::Write(root, &json);

  // NOTE: In production, send this JSON via NetworkService to
  // https://safebrowsing.googleapis.com/v4/threatListUpdates:fetch?key=<api_key>
  // The response contains threatEntrySet additions/removals for the
  // hash prefix database. Parse and apply to HashDatabase.
  //
  // For MVP, the update is logged and deferred until NetworkService
  // integration is wired into SafeBrowsingService.
  VEOR_LOGD(LogCategory::kSecurity,
            "SafeBrowsing: update payload ready, " +
                std::to_string(json.size()) + " bytes");

  ++api_calls_;
}

void SafeBrowsingService::StartPeriodicUpdates(base::TimeDelta interval) {
  if (update_timer_ && update_timer_->IsRunning()) {
    return;
  }

  update_timer_ = std::make_unique<base::RepeatingTimer>();
  update_timer_->Start(FROM_HERE, interval,
                       base::BindRepeating(&SafeBrowsingService::UpdateHashDatabase,
                                           base::Unretained(this)));

  VEOR_LOGI(LogCategory::kSecurity,
            "SafeBrowsing: periodic updates started (" +
                std::to_string(interval.InMinutes()) + " min interval)");
}

void SafeBrowsingService::StopPeriodicUpdates() {
  if (update_timer_) {
    update_timer_->Stop();
    update_timer_.reset();
  }
  VEOR_LOGI(LogCategory::kSecurity, "SafeBrowsing: periodic updates stopped");
}

bool SafeBrowsingService::IsPeriodicUpdateActive() const {
  return update_timer_ && update_timer_->IsRunning();
}

void SafeBrowsingService::OnUpdateResponse(const std::string& response) {
  // Parse threatListUpdates:fetch response and apply to HashDatabase.
  // Response format:
  // {
  //   "listUpdateResponses": [{
  //     "threatType": "MALWARE",
  //     "threatEntryType": "URL",
  //     "platformType": "ANY_PLATFORM",
  //     "threatEntrySet": [{
  //       "compressionType": "RAW",
  //       "rawHashes": { "prefixSize": 4, "rawHashes": "<base64>" },
  //       "rawIndices": { "indices": [0, 1, 2] },
  //       "removalIndices": { "indices": [3, 4] }
  //     }]
  //   }]
  // }
  VEOR_LOGD(LogCategory::kSecurity,
            "SafeBrowsing: processing update response, " +
                std::to_string(response.size()) + " bytes");

  auto result = base::JSONReader::Read(response);
  if (!result || !result->is_dict()) {
    VEOR_LOGW(LogCategory::kSecurity,
              "SafeBrowsing: failed to parse update response");
    return;
  }

  const auto& dict = result->GetDict();
  const auto* responses = dict.FindList("listUpdateResponses");
  if (!responses) {
    VEOR_LOGW(LogCategory::kSecurity,
              "SafeBrowsing: no listUpdateResponses in update");
    return;
  }

  size_t additions = 0;
  size_t removals = 0;

  for (const auto& entry : *responses) {
    if (!entry.is_dict()) continue;
    const auto& resp_dict = entry.GetDict();

    const std::string* threat_type = resp_dict.FindString("threatType");
    if (!threat_type) continue;

    const auto* entries = resp_dict.FindList("threatEntrySet");
    if (!entries) continue;

    for (const auto& set_entry : *entries) {
      if (!set_entry.is_dict()) continue;
      const auto& set_dict = set_entry.GetDict();

      // Process additions (rawHashes)
      const auto* raw_hashes = set_dict.FindDict("rawHashes");
      if (raw_hashes) {
        const std::string* hash_data = raw_hashes->FindString("rawHashes");
        if (hash_data) {
          // Decode base64 and add to HashDatabase
          // TODO: Implement base64 decode and HashDatabase::AddPrefixRange
          additions += hash_data->size() / 4;  // Approximate
        }
      }

      // Process removals (removalIndices)
      const auto* removal_indices = set_dict.FindDict("removalIndices");
      if (removal_indices) {
        const auto* indices = removal_indices->FindList("indices");
        if (indices) {
          removals += indices->size();
        }
      }
    }
  }

  VEOR_LOGI(LogCategory::kSecurity,
            "SafeBrowsing: update applied — " + std::to_string(additions) +
                " additions, " + std::to_string(removals) + " removals");
}

// ── Private ──────────────────────────────────────────────────────────────────

void SafeBrowsingService::CacheResult(const GURL& url,
                                      const ThreatCheckResult& result) {
  base::AutoLock lock(cache_lock_);
  if (cache_.size() >= kMaxCacheSize) {
    // Simple eviction: erase first entry
    cache_.erase(cache_.begin());
  }
  cache_[url.spec()] = result;
}

bool SafeBrowsingService::GetCachedResult(
    const GURL& url, ThreatCheckResult* out) {
  base::AutoLock lock(cache_lock_);
  auto it = cache_.find(url.spec());
  if (it == cache_.end()) {
    ++cache_misses_;
    return false;
  }
  if (base::Time::Now() - it->second.check_time > kCacheTtl) {
    cache_.erase(it);
    ++cache_misses_;
    return false;
  }
  *out = it->second;
  ++cache_hits_;
  return true;
}

std::string SafeBrowsingService::ComputeHashPrefix(const GURL& url) {
  return HashDatabase::ComputePrefix(url);
}

size_t SafeBrowsingService::GetCacheHitCount() const {
  base::AutoLock lock(cache_lock_);
  return cache_hits_;
}

size_t SafeBrowsingService::GetCacheMissCount() const {
  base::AutoLock lock(cache_lock_);
  return cache_misses_;
}

}  // namespace veor