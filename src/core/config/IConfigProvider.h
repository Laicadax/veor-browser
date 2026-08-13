// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ConfigValue
// ─────────────────────────────────────────────────────────────────────────────

using ConfigValue = std::variant<bool, int, double, std::string>;

// ─────────────────────────────────────────────────────────────────────────────
// IConfigProvider
// ─────────────────────────────────────────────────────────────────────────────
// Key-value configuration with in-memory cache and async disk persistence.
//
// Thread safety:
//   - Reads: [Any Thread] — backed by in-memory cache.
//   - Writes: [Any Thread] — enqueued to background thread for persistence.
//
// Keys use dot notation: "workspace.default_name", "tabs.sleep_delay_minutes".

class IConfigProvider {
 public:
  virtual ~IConfigProvider() = default;

  // ── Reads ──

  // Generic read. Returns default_value if key not found.
  // [Any Thread]
  virtual ConfigValue Get(const std::string& key,
                          const ConfigValue& default_value) const = 0;

  // Type-safe accessors.
  // [Any Thread]
  virtual bool GetBool(const std::string& key, bool default_value = false) const = 0;
  virtual int GetInt(const std::string& key, int default_value = 0) const = 0;
  virtual double GetDouble(const std::string& key,
                           double default_value = 0.0) const = 0;
  virtual std::string GetString(const std::string& key,
                                 const std::string& default_value = {}) const = 0;

  // ── Writes ──

  // Sets a value. Persists asynchronously.
  // [Any Thread]
  virtual void Set(const std::string& key, const ConfigValue& value) = 0;

  // Batch write (atomic in memory; persisted as a single transaction).
  // [Any Thread]
  virtual void SetAll(
      const std::vector<std::pair<std::string, ConfigValue>>& values) = 0;

  // ── Reset ──

  // Clears all user overrides, restoring defaults.
  // [Any Thread]
  virtual void ResetAll() = 0;

  // ── Persistence ──

  // Forces immediate save to disk. Blocks until complete.
  // [PostTask Required] — should be called from IO thread.
  virtual Result<void, std::string> SaveNow() = 0;

  // Loads from disk into memory cache. Called during initialization.
  // [PostTask Required]
  virtual Result<void, std::string> Load() = 0;
};

}  // namespace veor
