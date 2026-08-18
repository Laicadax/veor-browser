// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/config/ConfigProviderImpl.h"

#include <fstream>
#include <sstream>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "core/logging/VeorLogger.h"

namespace veor {

namespace {

// Converts ConfigValue to base::Value.
base::Value ConfigToBaseValue(const ConfigValue& value) {
  return std::visit(
      [](auto&& arg) -> base::Value {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>) {
          return base::Value(arg);
        } else if constexpr (std::is_same_v<T, int>) {
          return base::Value(arg);
        } else if constexpr (std::is_same_v<T, double>) {
          return base::Value(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
          return base::Value(arg);
        }
        return base::Value();
      },
      value);
}

// Converts base::Value to ConfigValue.
std::optional<ConfigValue> BaseToConfigValue(const base::Value* value) {
  if (!value)
    return std::nullopt;

  switch (value->type()) {
    case base::Value::Type::BOOLEAN:
      return ConfigValue(value->GetBool());
    case base::Value::Type::INTEGER:
      return ConfigValue(value->GetInt());
    case base::Value::Type::DOUBLE:
      return ConfigValue(value->GetDouble());
    case base::Value::Type::STRING:
      return ConfigValue(value->GetString());
    default:
      return std::nullopt;
  }
}

}  // namespace

ConfigProviderImpl::ConfigProviderImpl(std::unique_ptr<ITaskRunner> io_runner)
    : io_runner_(std::move(io_runner)) {}

ConfigProviderImpl::~ConfigProviderImpl() {
  // Ensure pending save completes.
  if (dirty_.load()) {
    SaveNow();
  }
}

void ConfigProviderImpl::SetFilePath(const std::string& path) {
  file_path_ = path;
}

ConfigProviderImpl::ConfigMap ConfigProviderImpl::ReadFromCache() const {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  return cache_;
}

void ConfigProviderImpl::WriteToCache(const ConfigMap& map) {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  cache_ = map;
}

ConfigValue ConfigProviderImpl::Get(
    const std::string& key,
    const ConfigValue& default_value) const {
  auto cache = ReadFromCache();
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }
  return default_value;
}

bool ConfigProviderImpl::GetBool(const std::string& key,
                                 bool default_value) const {
  auto value = Get(key, ConfigValue(default_value));
  if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value);
  }
  return default_value;
}

int ConfigProviderImpl::GetInt(const std::string& key,
                              int default_value) const {
  auto value = Get(key, ConfigValue(default_value));
  if (std::holds_alternative<int>(value)) {
    return std::get<int>(value);
  }
  return default_value;
}

double ConfigProviderImpl::GetDouble(const std::string& key,
                                     double default_value) const {
  auto value = Get(key, ConfigValue(default_value));
  if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  }
  return default_value;
}

std::string ConfigProviderImpl::GetString(
    const std::string& key,
    const std::string& default_value) const {
  auto value = Get(key, ConfigValue(default_value));
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  }
  return default_value;
}

void ConfigProviderImpl::Set(const std::string& key,
                             const ConfigValue& value) {
  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_[key] = value;
  }
  dirty_.store(true);
  PostSave();
}

void ConfigProviderImpl::SetAll(
    const std::vector<std::pair<std::string, ConfigValue>>& values) {
  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    for (const auto& [key, value] : values) {
      cache_[key] = value;
    }
  }
  dirty_.store(true);
  PostSave();
}

void ConfigProviderImpl::ResetAll() {
  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
  }
  dirty_.store(true);
  PostSave();
}

void ConfigProviderImpl::PostSave() {
  bool expected = false;
  if (!save_pending_.compare_exchange_strong(expected, true)) {
    return;  // Save already pending.
  }

  io_runner_->PostTask(base::BindOnce(&ConfigProviderImpl::DoSave,
                                       base::Unretained(this)));
}

Result<void, std::string> ConfigProviderImpl::SaveNow() {
  return DoSave();
}

Result<void, std::string> ConfigProviderImpl::DoSave() {
  save_pending_.store(false);

  if (file_path_.empty()) {
    return Result<void, std::string>::Err("No file path set");
  }

  if (!dirty_.load()) {
    return Result<void, std::string>::Ok();
  }

  auto cache = ReadFromCache();

  base::Value::Dict root;
  for (const auto& [key, value] : cache) {
    root.Set(key, ConfigToBaseValue(value));
  }

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          base::Value(std::move(root)),
          base::JSONWriter::OPTIONS_PRETTY_PRINT,
          &json)) {
    VEOR_LOGE(LogCategory::kCore, "Failed to serialize config to JSON");
    return Result<void, std::string>::Err("JSON serialization failed");
  }

  std::ofstream file(file_path_);
  if (!file.is_open()) {
    VEOR_LOGE(LogCategory::kCore,
              "Failed to open config file: " + file_path_);
    return Result<void, std::string>::Err("Failed to open file: " + file_path_);
  }

  file << json;
  file.close();

  dirty_.store(false);
  VEOR_LOGI(LogCategory::kCore,
            "Config saved to " + file_path_ + " (" +
                std::to_string(cache.size()) + " entries)");
  return Result<void, std::string>::Ok();
}

Result<void, std::string> ConfigProviderImpl::Load() {
  return DoLoad();
}

Result<void, std::string> ConfigProviderImpl::DoLoad() {
  if (file_path_.empty()) {
    return Result<void, std::string>::Err("No file path set");
  }

  std::ifstream file(file_path_);
  if (!file.is_open()) {
    // File doesn't exist yet — not an error, just empty state.
    VEOR_LOGI(LogCategory::kCore,
              "Config file not found, starting with empty config: " +
                  file_path_);
    return Result<void, std::string>::Ok();
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  auto result = base::JSONReader::ReadAndReturnValueWithError(buffer.str());
  if (!result.has_value()) {
    VEOR_LOGE(LogCategory::kCore,
              "Failed to parse config JSON: " + result.error().message);
    return Result<void, std::string>::Err("JSON parse error: " +
                                          result.error().message);
  }

  const base::Value::Dict* root = result.Unwrap().GetIfDict();
  if (!root) {
    return Result<void, std::string>::Err("Config root is not an object");
  }

  ConfigMap new_cache;
  for (const auto [key, value] : *root) {
    auto config_value = BaseToConfigValue(&value);
    if (config_value) {
      new_cache.emplace(key, std::move(*config_value));
    }
  }

  WriteToCache(new_cache);
  dirty_.store(false);

  VEOR_LOGI(LogCategory::kCore,
            "Config loaded from " + file_path_ + " (" +
                std::to_string(new_cache.size()) + " entries)");
  return Result<void, std::string>::Ok();
}

}  // namespace veor
