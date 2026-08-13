// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings/SettingsProviderImpl.h"

#include "core/config/IConfigProvider.h"
#include "core/logging/VeorLogger.h"
#include "settings/SettingsSchema.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// Value conversion helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

SettingValue ConfigToSetting(const ConfigValue& value) {
  return std::visit(
      [](auto&& arg) -> SettingValue {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> ||
                      std::is_same_v<T, double> || std::is_same_v<T, std::string>) {
          return SettingValue(arg);
        }
        return SettingValue{};
      },
      value);
}

ConfigValue SettingToConfig(const SettingValue& value) {
  return std::visit(
      [](auto&& arg) -> ConfigValue {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> ||
                      std::is_same_v<T, double> || std::is_same_v<T, std::string>) {
          return ConfigValue(arg);
        }
        return ConfigValue{};
      },
      value);
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// SettingsProviderImpl
// ─────────────────────────────────────────────────────────────────────────────

SettingsProviderImpl::SettingsProviderImpl(IConfigProvider* config)
    : config_(config) {}

bool SettingsProviderImpl::GetBool(const std::string& key,
                                 bool default_value) const {
  auto result = GetValue(key);
  if (result.IsOk() && std::holds_alternative<bool>(result.Unwrap())) {
    return std::get<bool>(result.Unwrap());
  }
  return default_value;
}

int SettingsProviderImpl::GetInt(const std::string& key,
                                int default_value) const {
  auto result = GetValue(key);
  if (result.IsOk() && std::holds_alternative<int>(result.Unwrap())) {
    return std::get<int>(result.Unwrap());
  }
  return default_value;
}

double SettingsProviderImpl::GetDouble(const std::string& key,
                                       double default_value) const {
  auto result = GetValue(key);
  if (result.IsOk() && std::holds_alternative<double>(result.Unwrap())) {
    return std::get<double>(result.Unwrap());
  }
  return default_value;
}

std::string SettingsProviderImpl::GetString(
    const std::string& key,
    const std::string& default_value) const {
  auto result = GetValue(key);
  if (result.IsOk() && std::holds_alternative<std::string>(result.Unwrap())) {
    return std::get<std::string>(result.Unwrap());
  }
  return default_value;
}

std::vector<std::string> SettingsProviderImpl::GetList(
    const std::string& key) const {
  auto result = GetValue(key);
  if (result.IsOk() &&
      std::holds_alternative<std::vector<std::string>>(result.Unwrap())) {
    return std::get<std::vector<std::string>>(result.Unwrap());
  }
  return {};
}

Result<SettingValue, std::string> SettingsProviderImpl::GetValue(
    const std::string& key) const {
  auto& schema = SettingsSchema::GetInstance();
  const SettingDef* def = schema.GetDefinition(key);
  if (!def) {
    return Result<SettingValue, std::string>::Err("Unknown setting: " + key);
  }

  // Try to read from config
  ConfigValue config_default = SettingToConfig(def->default_value);
  ConfigValue stored = config_->Get(key, config_default);

  // If stored equals default, return default
  if (stored == config_default) {
    return Result<SettingValue, std::string>::Ok(def->default_value);
  }

  SettingValue value = ConfigToSetting(stored);
  return Result<SettingValue, std::string>::Ok(value);
}

Result<void, std::string> SettingsProviderImpl::SetValue(
    const std::string& key,
    const SettingValue& value) {
  auto& schema = SettingsSchema::GetInstance();

  // Validate
  auto validation = schema.ValidateValue(key, value);
  if (validation.IsErr()) {
    return Result<void, std::string>::Err(validation.UnwrapErr());
  }

  // Get old value for observers
  auto old_value_result = GetValue(key);
  SettingValue old_value = old_value_result.IsOk()
                               ? old_value_result.Unwrap()
                               : schema.GetDefaultValue(key);

  // Store
  config_->Set(key, SettingToConfig(value));

  // Notify
  NotifyObservers(key, value, old_value);

  return Result<void, std::string>::Ok();
}

Result<void, std::string> SettingsProviderImpl::SetValues(
    const std::vector<std::pair<std::string, SettingValue>>& values) {
  auto& schema = SettingsSchema::GetInstance();

  // Validate all first
  for (const auto& [key, value] : values) {
    auto validation = schema.ValidateValue(key, value);
    if (validation.IsErr()) {
      return Result<void, std::string>::Err(validation.UnwrapErr());
    }
  }

  // Store all
  std::vector<std::pair<std::string, ConfigValue>> config_values;
  config_values.reserve(values.size());
  for (const auto& [key, value] : values) {
    config_values.emplace_back(key, SettingToConfig(value));
  }
  config_->SetAll(config_values);

  // Notify
  for (const auto& [key, value] : values) {
    auto old_value_result = GetValue(key);
    SettingValue old_value = old_value_result.IsOk()
                                 ? old_value_result.Unwrap()
                                 : schema.GetDefaultValue(key);
    NotifyObservers(key, value, old_value);
  }

  return Result<void, std::string>::Ok();
}

const SettingDef* SettingsProviderImpl::GetDefinition(
    const std::string& key) const {
  return SettingsSchema::GetInstance().GetDefinition(key);
}

std::vector<const SettingDef*> SettingsProviderImpl::GetDefinitionsByCategory(
    const std::string& category) const {
  return SettingsSchema::GetInstance().GetByCategory(category);
}

std::vector<std::string> SettingsProviderImpl::GetCategories() const {
  return SettingsSchema::GetInstance().GetCategories();
}

SettingScope SettingsProviderImpl::GetScope(const std::string& key) const {
  const SettingDef* def = GetDefinition(key);
  return def ? def->scope : SettingScope::kGlobal;
}

void SettingsProviderImpl::ResetToDefault(const std::string& key) {
  auto& schema = SettingsSchema::GetInstance();
  SettingValue default_value = schema.GetDefaultValue(key);

  auto old_value_result = GetValue(key);
  SettingValue old_value = old_value_result.IsOk()
                               ? old_value_result.Unwrap()
                               : default_value;

  config_->Set(key, SettingToConfig(default_value));
  NotifyObservers(key, default_value, old_value);
}

void SettingsProviderImpl::ResetCategoryToDefaults(const std::string& category) {
  auto defs = GetDefinitionsByCategory(category);
  for (const auto* def : defs) {
    ResetToDefault(def->string_key);
  }
}

void SettingsProviderImpl::ResetAllToDefaults() {
  auto categories = GetCategories();
  for (const auto& category : categories) {
    ResetCategoryToDefaults(category);
  }
}

void SettingsProviderImpl::AddObserver(const std::string& key,
                                      SettingsObserver* observer) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  key_observers_[key].insert(observer);
}

void SettingsProviderImpl::RemoveObserver(const std::string& key,
                                         SettingsObserver* observer) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  auto it = key_observers_.find(key);
  if (it != key_observers_.end()) {
    it->second.erase(observer);
    if (it->second.empty()) {
      key_observers_.erase(it);
    }
  }
}

void SettingsProviderImpl::AddGlobalObserver(SettingsObserver* observer) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  global_observers_.insert(observer);
}

void SettingsProviderImpl::RemoveGlobalObserver(SettingsObserver* observer) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  global_observers_.erase(observer);
}

Result<void, std::string> SettingsProviderImpl::Save() {
  // ConfigProvider handles persistence
  return Result<void, std::string>::Ok();
}

Result<void, std::string> SettingsProviderImpl::Load() {
  // ConfigProvider handles loading
  return Result<void, std::string>::Ok();
}

void SettingsProviderImpl::NotifyObservers(const std::string& key,
                                          const SettingValue& new_value,
                                          const SettingValue& old_value) {
  std::lock_guard<std::mutex> lock(observers_mutex_);

  // Key-specific observers
  auto it = key_observers_.find(key);
  if (it != key_observers_.end()) {
    for (auto* observer : it->second) {
      observer->OnSettingChanged(key, new_value, old_value);
    }
  }

  // Global observers
  for (auto* observer : global_observers_) {
    observer->OnSettingChanged(key, new_value, old_value);
  }
}

}  // namespace veor
