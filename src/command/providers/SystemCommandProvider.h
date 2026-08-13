// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base/callback.h"
#include "command/ICommandProvider.h"
#include "platform/IDefaultBrowserRegistrar.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SystemCommandProvider
// ─────────────────────────────────────────────────────────────────────────────
// Provides system-level commands in the Command Palette:
//   - Set VEOR as default browser
//   - Open default apps settings
//   - Toggle Reader Mode

class SystemCommandProvider : public ICommandProvider {
 public:
  SystemCommandProvider(
      std::unique_ptr<IDefaultBrowserRegistrar> registrar,
      base::RepeatingClosure on_reader_mode);
  ~SystemCommandProvider() override = default;

  std::vector<CommandItem> Query(const std::string& query) override;
  Result<void, std::string> Execute(CommandId id) override;

 private:
  std::unique_ptr<IDefaultBrowserRegistrar> registrar_;
  base::RepeatingClosure on_reader_mode_;
  CommandId set_default_id_;
  CommandId open_settings_id_;
  CommandId reader_toggle_id_;
};

}  // namespace veor
