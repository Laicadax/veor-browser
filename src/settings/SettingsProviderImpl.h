// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "settings/ISettingsProvider.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace veor {

class IConfigProvider;

// ─────────────────────────────────────────────────────────────────────────────
// SettingsProviderImpl
// ─────────────────────────────────────────────────────────────────────────────
// Schema-driven settings backed by IConfigProvider for persistence.
//
// Thread safety: [UI Thread] for reads and writes.

class SettingsProviderImpl : public ISettingsProvider {
 public:
  explicit SettingsProviderImpl(IConfigProvider* config);
  ~SettingsProviderImpl() override = default;

  // ISettingsProvider
  bool GetBool(const std::string& key, bool default_value) const override;
  int GetInt(const std::string& key, int default_value) const override;
  double GetDouble(const std::string& key, double default_value) const override;
  std::string GetString(const std::string& key,
                       const std::string& default_value) const override;
  std::vector<std::string> GetList(const std::string& key) const override;
  Result<SettingValue, std::string> GetValue(const std::string& key) const override;

  Result<void, std::string> SetValue(const std::string& key,
                                      const SettingValue& value) override;
  Result<void, std::string> SetValues(
      const std::vector<std::pair<std::string, SettingValue>>& values) override;

  const SettingDef* GetDefinition(const std::string& key) const override;
  std::vector<const SettingDef*> GetDefinitionsByCategory(
      const std::string& category) const override;
  std::vector<std::string> GetCategories() const override;
  SettingScope GetScope(const std::string& key) const override;

  void ResetToDefault(const std::string& key) override;
  void ResetCategoryToDefaults(const std::string& category) override;
  void ResetAllToDefaults() override;

  void AddObserver(const std::string& key, SettingsObserver* observer) override;
  void RemoveObserver(const std::string& key, SettingsObserver* observer) override;
  void AddGlobalObserver(SettingsObserver* observer) override;
  void RemoveGlobalObserver(SettingsObserver* observer) override;

  Result<void, std::string> Save() override;
  Result<void, std::string> Load() override;

 private:
  void NotifyObservers(const std::string& key,
                      const SettingValue& new_value,
                      const SettingValue& old_value);
  SettingValue ConfigToSettingValue(const ConfigValue& value) const;
  ConfigValue SettingToConfigValue(const SettingValue& value) const;

  IConfigProvider* config_;

  mutable std::mutex observers_mutex_;
  std::unordered_map<std::string, std::unordered_set<SettingsObserver*>> key_observers_;
  std::unordered_set<SettingsObserver*> global_observers_;
};

}  // namespace veor
