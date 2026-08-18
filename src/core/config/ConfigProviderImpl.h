// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
class Workspace;

#include "core/config/IConfigProvider.h"
#include "core/threading/ITaskRunner.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ConfigProviderImpl
// ─────────────────────────────────────────────────────────────────────────────
// In-memory config store with JSON file persistence.
//
// Persistence format: JSON object with dot-notation keys flattened.
// Example: {"workspace.default_name": "Personal", "tabs.sleep_delay_minutes": 5}
//
// Thread safety: Reads are lock-free (atomic shared_ptr swap).
//                Writes take a mutex and post persistence to IO runner.

class ConfigProviderImpl : public IConfigProvider {
 public:
  explicit ConfigProviderImpl(std::unique_ptr<ITaskRunner> io_runner);
  ~ConfigProviderImpl() override;

  // IConfigProvider
  ConfigValue Get(const std::string& key,
                  const ConfigValue& default_value) const override;
  bool GetBool(const std::string& key, bool default_value) const override;
  int GetInt(const std::string& key, int default_value) const override;
  double GetDouble(const std::string& key, double default_value) const override;
  std::string GetString(const std::string& key,
                       const std::string& default_value) const override;

  void Set(const std::string& key, const ConfigValue& value) override;
  void SetAll(
      const std::vector<std::pair<std::string, ConfigValue>>& values) override;
  void ResetAll() override;

  Result<void, std::string> SaveNow() override;
  Result<void, std::string> Load() override;

  // Sets the file path for persistence. Must be called before Load/Save.
  void SetFilePath(const std::string& path);

 private:
  using ConfigMap = std::unordered_map<std::string, ConfigValue>;

  ConfigMap ReadFromCache() const;
  void WriteToCache(const ConfigMap& map);
  void PostSave();
  Result<void, std::string> DoSave();
  Result<void, std::string> DoLoad();

  std::string file_path_;
  std::unique_ptr<ITaskRunner> io_runner_;

  // Cache: protected by atomic shared_ptr swap for reads, mutex for writes.
  mutable std::mutex cache_mutex_;
  ConfigMap cache_;
  std::atomic<bool> dirty_{false};
  std::atomic<bool> save_pending_{false};
};

}  // namespace veor
