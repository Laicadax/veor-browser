// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ISettingsUI
// ─────────────────────────────────────────────────────────────────────────────
// Settings in V10 are not a page. They are a stream through Command Palette:
//   > theme
//   > sleep tabs
//   > hardware acceleration
//
// This interface exists for future extensibility should a dedicated
// settings overlay ever be required. For now, it remains minimal.
// ─────────────────────────────────────────────────────────────────────────────

class ISettingsUI {
 public:
  virtual ~ISettingsUI() = default;

  virtual void ToggleSetting(const std::string& key) = 0;
  virtual void ResetAll() = 0;
};

}  // namespace veor
