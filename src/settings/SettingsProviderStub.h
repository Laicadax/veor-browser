#pragma once

#include "settings/ISettingsProvider.h"

namespace veor {

class SettingsProviderStub : public ISettingsProvider {
 public:
  bool GetBool(const std::string& key, bool default_value = false) const override {
    return default_value;
  }

  int GetInt(const std::string& key, int default_value = 0) const override {
    return default_value;
  }

  double GetDouble(const std::string& key, double default_value = 0.0) const override {
    return default_value;
  }

  std::string GetString(const std::string& key,
                         const std::string& default_value = {}) const override {
    return default_value;
  }

  std::vector<std::string> GetList(const std::string& key) const override {
    return {};
  }

  Result<SettingValue, std::string> GetValue(const std::string& key) const override {
    return Result<SettingValue, std::string>::Err("Not found");
  }

  Result<void, std::string> SetValue(const std::string& key,
                                      const SettingValue& value) override {
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> SetValues(
      const std::vector<std::pair<std::string, SettingValue>>& values) override {
    return Result<void, std::string>::Ok();
  }

  const SettingDef* GetDefinition(const std::string& key) const override {
    return nullptr;
  }

  std::vector<const SettingDef*> GetDefinitionsByCategory(
      const std::string& category) const override {
    return {};
  }

  std::vector<std::string> GetCategories() const override {
    return {};
  }

  SettingScope GetScope(const std::string& key) const override {
    return SettingScope::kGlobal;
  }

  void ResetToDefault(const std::string& key) override {}
  void ResetCategoryToDefaults(const std::string& category) override {}
  void ResetAllToDefaults() override {}

  void AddObserver(const std::string& key, SettingsObserver* observer) override {}
  void RemoveObserver(const std::string& key, SettingsObserver* observer) override {}
  void AddGlobalObserver(SettingsObserver* observer) override {}
  void RemoveGlobalObserver(SettingsObserver* observer) override {}

  Result<void, std::string> Save() override {
    return Result<void, std::string>::Ok();
  }
  Result<void, std::string> Load() override {
    return Result<void, std::string>::Ok();
  }
};

}  // namespace veor
