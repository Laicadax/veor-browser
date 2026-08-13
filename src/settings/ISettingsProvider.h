// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <string>
#include <variant>
#include <vector>

#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// Setting Types
// ─────────────────────────────────────────────────────────────────────────────

enum class SettingType {
  kBool,
  kInt,
  kDouble,
  kString,
  kEnum,
  kList,
  kGroup
};

enum class SettingScope {
  kGlobal,    // Applies across all workspaces
  kWorkspace  // Per-workspace override
};

enum class SettingChangeMode {
  kInstant,        // Apply immediately
  kOnConfirm,      // Apply on explicit confirmation
  kRequiresRestart // Apply after browser restart
};

struct Range {
  int min = 0;
  int max = 0;
};

using SettingValue = std::variant<bool, int, double, std::string, std::vector<std::string>>;

struct SettingDef {
  std::string string_key;           // Human-readable key, e.g. "appearance.theme"
  SettingType type;
  SettingValue default_value;
  std::optional<Range> range;
  std::vector<std::string> enum_values;
  SettingScope scope;
  SettingChangeMode change_mode;
  std::string category;             // UI grouping
  std::string description;          // Tooltip / help text
  bool requires_restart = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// SettingsObserver
// ─────────────────────────────────────────────────────────────────────────────

class SettingsObserver {
 public:
  virtual ~SettingsObserver() = default;
  virtual void OnSettingChanged(const std::string& key,
                                 const SettingValue& new_value,
                                 const SettingValue& old_value) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// ISettingsProvider
// ─────────────────────────────────────────────────────────────────────────────
// Schema-driven settings with validation and observers.
//
// Thread safety: [UI Thread] for reads and writes.
//                Persistence is async on [IO Thread].

class ISettingsProvider {
 public:
  virtual ~ISettingsProvider() = default;

  // ── Reads ──
  virtual bool GetBool(const std::string& key, bool default_value = false) const = 0;
  virtual int GetInt(const std::string& key, int default_value = 0) const = 0;
  virtual double GetDouble(const std::string& key, double default_value = 0.0) const = 0;
  virtual std::string GetString(const std::string& key,
                                 const std::string& default_value = {}) const = 0;
  virtual std::vector<std::string> GetList(const std::string& key) const = 0;

  // Generic read
  virtual Result<SettingValue, std::string> GetValue(const std::string& key) const = 0;

  // ── Writes ──
  virtual Result<void, std::string> SetValue(const std::string& key,
                                              const SettingValue& value) = 0;

  // Batch write (atomic validation, all-or-nothing apply)
  virtual Result<void, std::string> SetValues(
      const std::vector<std::pair<std::string, SettingValue>>& values) = 0;

  // ── Schema ──
  virtual const SettingDef* GetDefinition(const std::string& key) const = 0;
  virtual std::vector<const SettingDef*> GetDefinitionsByCategory(
      const std::string& category) const = 0;
  virtual std::vector<std::string> GetCategories() const = 0;

  // ── Scope ──
  virtual SettingScope GetScope(const std::string& key) const = 0;

  // ── Reset ──
  virtual void ResetToDefault(const std::string& key) = 0;
  virtual void ResetCategoryToDefaults(const std::string& category) = 0;
  virtual void ResetAllToDefaults() = 0;

  // ── Observers ──
  virtual void AddObserver(const std::string& key, SettingsObserver* observer) = 0;
  virtual void RemoveObserver(const std::string& key, SettingsObserver* observer) = 0;
  virtual void AddGlobalObserver(SettingsObserver* observer) = 0;
  virtual void RemoveGlobalObserver(SettingsObserver* observer) = 0;

  // ── Persistence ──
  virtual Result<void, std::string> Save() = 0;
  virtual Result<void, std::string> Load() = 0;
};

}  // namespace veor
