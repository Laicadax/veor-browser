// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings/SettingsSchema.h"
#include "workspace/Workspace.h"

#include "core/logging/VeorLogger.h"

namespace veor {

SettingsSchema& SettingsSchema::GetInstance() {
  static SettingsSchema instance;
  return instance;
}

void SettingsSchema::Register(const SettingDef& def) {
  auto [it, inserted] = definitions_.try_emplace(def.string_key, def);
  if (!inserted) {
    VEOR_LOGE(LogCategory::kSettings,
              "Duplicate setting key registered: " + def.string_key);
  }
}

void SettingsSchema::RegisterCategory(const std::string& category,
                                      const std::vector<SettingDef>& defs) {
  for (const auto& def : defs) {
    Register(def);
  }
}

const SettingDef* SettingsSchema::GetDefinition(const std::string& key) const {
  auto it = definitions_.find(key);
  if (it != definitions_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<const SettingDef*> SettingsSchema::GetByCategory(
    const std::string& category) const {
  std::vector<const SettingDef*> result;
  for (const auto& [key, def] : definitions_) {
    if (def.category == category) {
      result.push_back(&def);
    }
  }
  return result;
}

std::vector<std::string> SettingsSchema::GetCategories() const {
  std::vector<std::string> categories;
  for (const auto& [key, def] : definitions_) {
    if (std::find(categories.begin(), categories.end(), def.category) ==
        categories.end()) {
      categories.push_back(def.category);
    }
  }
  return categories;
}

Result<void, std::string> SettingsSchema::ValidateValue(
    const std::string& key,
    const SettingValue& value) const {
  const SettingDef* def = GetDefinition(key);
  if (!def) {
    return Result<void, std::string>::Err("Unknown setting key: " + key);
  }

  // Type validation
  bool type_ok = false;
  switch (def->type) {
    case SettingType::kBool:
      type_ok = std::holds_alternative<bool>(value);
      break;
    case SettingType::kInt:
      type_ok = std::holds_alternative<int>(value);
      break;
    case SettingType::kDouble:
      type_ok = std::holds_alternative<double>(value);
      break;
    case SettingType::kString:
    case SettingType::kEnum:
      type_ok = std::holds_alternative<std::string>(value);
      break;
    case SettingType::kList:
      type_ok = std::holds_alternative<std::vector<std::string>>(value);
      break;
    case SettingType::kGroup:
      type_ok = false;  // Groups cannot have direct values
      break;
  }

  if (!type_ok) {
    return Result<void, std::string>::Err(
        "Type mismatch for setting: " + key);
  }

  // Range validation for integers
  if (def->type == SettingType::kInt && def->range) {
    int int_val = std::get<int>(value);
    if (int_val < def->range->min || int_val > def->range->max) {
      return Result<void, std::string>::Err(
          "Value out of range for " + key + ": " + std::to_string(int_val) +
          " not in [" + std::to_string(def->range->min) + ", " +
          std::to_string(def->range->max) + "]");
    }
  }

  // Enum validation
  if (def->type == SettingType::kEnum) {
    const std::string& str_val = std::get<std::string>(value);
    if (std::find(def->enum_values.begin(), def->enum_values.end(),
                  str_val) == def->enum_values.end()) {
      return Result<void, std::string>::Err(
          "Invalid enum value for " + key + ": " + str_val);
    }
  }

  return Result<void, std::string>::Ok();
}

SettingValue SettingsSchema::GetDefaultValue(const std::string& key) const {
  const SettingDef* def = GetDefinition(key);
  if (def) {
    return def->default_value;
  }
  return SettingValue{};
}

bool SettingsSchema::HasKey(const std::string& key) const {
  return definitions_.find(key) != definitions_.end();
}

// ─────────────────────────────────────────────────────────────────────────────
// Built-in schema
// ─────────────────────────────────────────────────────────────────────────────

void RegisterBuiltInSettingsSchema() {
  auto& schema = SettingsSchema::GetInstance();

  schema.RegisterCategory("Appearance", {
    {
      .string_key = "appearance.theme",
      .type = SettingType::kEnum,
      .default_value = std::string("dark"),
      .enum_values = {"dark", "light", "system"},
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Appearance",
      .description = "The color theme of the browser interface."
    },
    {
      .string_key = "appearance.accent_color",
      .type = SettingType::kString,
      .default_value = std::string("#8B0000"),
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Appearance",
      .description = "Custom accent color in hex format."
    }
  });

  schema.RegisterCategory("Tabs", {
    {
      .string_key = "tabs.sleep_delay_minutes",
      .type = SettingType::kInt,
      .default_value = 5,
      .range = Range{.min = 1, .max = 120},
      .scope = SettingScope::kWorkspace,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Tabs",
      .description = "Minutes of inactivity before a background tab is suspended."
    },
    {
      .string_key = "tabs.max_sleeping",
      .type = SettingType::kInt,
      .default_value = 50,
      .range = Range{.min = 10, .max = 500},
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Tabs",
      .description = "Maximum number of sleeping tabs before forced cleanup."
    }
  });

  schema.RegisterCategory("Workspace", {
    {
      .string_key = "workspace.default_name",
      .type = SettingType::kString,
      .default_value = std::string("Personal"),
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Workspace",
      .description = "Default name for new workspaces."
    },
    {
      .string_key = "workspace.restore_on_startup",
      .type = SettingType::kBool,
      .default_value = true,
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kRequiresRestart,
      .category = "Workspace",
      .description = "Restore previous workspaces on browser startup."
    }
  });

  schema.RegisterCategory("Privacy", {
    {
      .string_key = "privacy.tracker_blocking",
      .type = SettingType::kBool,
      .default_value = true,
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Privacy",
      .description = "Block known trackers and analytics scripts."
    },
    {
      .string_key = "privacy.https_only",
      .type = SettingType::kBool,
      .default_value = true,
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Privacy",
      .description = "Upgrade HTTP connections to HTTPS when possible."
    }
  });

  schema.RegisterCategory("Search", {
    {
      .string_key = "search.default_engine",
      .type = SettingType::kEnum,
      .default_value = std::string("google"),
      .enum_values = {"google", "duckduckgo", "bing"},
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Search",
      .description = "Default search engine for omnibox queries."
    }
  });

  schema.RegisterCategory("Performance", {
    {
      .string_key = "performance.target_fps",
      .type = SettingType::kInt,
      .default_value = 120,
      .range = Range{.min = 30, .max = 240},
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Performance",
      .description = "Target frame rate for UI animations."
    },
    {
      .string_key = "performance.memory_limit_mb",
      .type = SettingType::kInt,
      .default_value = 2048,
      .range = Range{.min = 512, .max = 8192},
      .scope = SettingScope::kGlobal,
      .change_mode = SettingChangeMode::kInstant,
      .category = "Performance",
      .description = "Memory limit before tab sleep is triggered."
    }
  });

  VEOR_LOGI(LogCategory::kSettings,
            "Built-in settings schema registered.");
}

}  // namespace veor
