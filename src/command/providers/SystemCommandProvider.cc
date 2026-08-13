// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "command/providers/SystemCommandProvider.h"

#include <algorithm>

#include "core/logging/VeorLogger.h"

namespace veor {

SystemCommandProvider::SystemCommandProvider(
    std::unique_ptr<IDefaultBrowserRegistrar> registrar,
    base::RepeatingClosure on_reader_mode)
    : registrar_(std::move(registrar)),
      on_reader_mode_(std::move(on_reader_mode)),
      set_default_id_(IdGenerator::NextId<CommandTag>()),
      open_settings_id_(IdGenerator::NextId<CommandTag>()),
      reader_toggle_id_(IdGenerator::NextId<CommandTag>()) {}

std::vector<CommandItem> SystemCommandProvider::Query(const std::string& query) {
  std::vector<CommandItem> results;

  if (!registrar_)
    return results;

  std::string lower_query = query;
  std::transform(lower_query.begin(), lower_query.end(),
                 lower_query.begin(), ::tolower);

  // "Set as default browser" command
  if (!registrar_->IsDefault()) {
    bool match = query.empty() ||
                 lower_query.find("default") != std::string::npos ||
                 lower_query.find("browser") != std::string::npos;
    if (match) {
      CommandItem item;
      item.id = set_default_id_;
      item.title = "Set VEOR as default browser";
      item.subtitle = registrar_->GetStatusMessage();
      item.category = "System";
      item.score = query.empty() ? 80 : 100;
      results.push_back(item);
    }
  }

  // "Open default apps settings" command
  bool match_settings = query.empty() ||
                        lower_query.find("default") != std::string::npos ||
                        lower_query.find("apps") != std::string::npos ||
                        lower_query.find("settings") != std::string::npos;
  if (match_settings) {
    CommandItem item;
    item.id = open_settings_id_;
    item.title = "Open default apps settings";
    item.subtitle = "Windows Settings";
    item.category = "System";
    item.score = query.empty() ? 60 : 90;
    results.push_back(item);
  }

  // "Toggle Reader Mode" command
  bool match_reader = query.empty() ||
                      lower_query.find("reader") != std::string::npos ||
                      lower_query.find("read") != std::string::npos ||
                      lower_query.find("distraction") != std::string::npos;
  if (match_reader) {
    CommandItem item;
    item.id = reader_toggle_id_;
    item.title = "Toggle Reader Mode";
    item.subtitle = "Distraction-free reading";
    item.category = "View";
    item.score = query.empty() ? 70 : 100;
    results.push_back(item);
  }

  return results;
}

Result<void, std::string> SystemCommandProvider::Execute(CommandId id) {
  if (!registrar_) {
    return Result<void, std::string>::Err("Default browser registrar not available");
  }

  if (id == set_default_id_) {
    if (registrar_->Register()) {
      VEOR_LOGI(LogCategory::kPlatform, "VEOR set as default browser");
      return Result<void, std::string>::Ok();
    }
    return Result<void, std::string>::Err("Failed to set VEOR as default browser");
  }

  if (id == open_settings_id_) {
    registrar_->OpenDefaultAppsSettings();
    return Result<void, std::string>::Ok();
  }

  if (id == reader_toggle_id_) {
    if (on_reader_mode_) {
      on_reader_mode_.Run();
      return Result<void, std::string>::Ok();
    }
    return Result<void, std::string>::Err("Reader mode not available");
  }

  return Result<void, std::string>::Err("Unknown system command");
}

}  // namespace veor
