// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "settings/ISettingsProvider.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SettingsSchema
// ─────────────────────────────────────────────────────────────────────────────
// Singleton registry of all valid settings. Immutable after initialization.
//
// Usage:
//   auto& schema = SettingsSchema::GetInstance();
//   schema.Register({"appearance.theme", SettingType::kEnum, ...});

class SettingsSchema {
 public:
  static SettingsSchema& GetInstance();

  // Registers a setting definition. Duplicate keys are rejected with DCHECK.
  void Register(const SettingDef& def);

  // Bulk registration for a category.
  void RegisterCategory(const std::string& category,
                        const std::vector<SettingDef>& defs);

  // Queries
  const SettingDef* GetDefinition(const std::string& key) const;
  std::vector<const SettingDef*> GetByCategory(const std::string& category) const;
  std::vector<std::string> GetCategories() const;

  // Validation
  Result<void, std::string> ValidateValue(const std::string& key,
                                          const SettingValue& value) const;

  // Returns the default value for a key.
  SettingValue GetDefaultValue(const std::string& key) const;

  // Returns true if the key is registered.
  bool HasKey(const std::string& key) const;

 private:
  SettingsSchema() = default;
  std::unordered_map<std::string, SettingDef> definitions_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Built-in schema registration
// ─────────────────────────────────────────────────────────────────────────────

void RegisterBuiltInSettingsSchema();

}  // namespace veor
