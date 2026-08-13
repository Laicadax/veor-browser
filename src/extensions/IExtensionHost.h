// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "core/base/VeorId.h"
#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ExtensionManifest — Parsed manifest.json (MV3 subset)
// ─────────────────────────────────────────────────────────────────────────────

struct ExtensionManifest {
  std::string name;
  std::string version;
  std::string description;
  std::vector<std::string> permissions;
  std::vector<std::string> host_permissions;
  bool has_background_script = false;
  bool has_content_scripts = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// IExtensionHost
// ─────────────────────────────────────────────────────────────────────────────
// Loads, isolates, and bridges Chrome Extension MV3.
// ─────────────────────────────────────────────────────────────────────────────

class IExtensionHost {
 public:
  virtual ~IExtensionHost() = default;

  // Load an unpacked extension from disk
  virtual Result<ExtensionId, std::string> LoadExtension(
      const std::string& path) = 0;

  // Unload by ID
  virtual Result<void, std::string> UnloadExtension(ExtensionId id) = 0;

  // Query loaded extensions
  virtual std::vector<ExtensionId> GetLoadedExtensions() const = 0;
  virtual const ExtensionManifest* GetManifest(ExtensionId id) const = 0;

  // Enable/disable without unloading
  virtual Result<void, std::string> EnableExtension(ExtensionId id, bool enabled) = 0;
  virtual bool IsEnabled(ExtensionId id) const = 0;
};

}  // namespace veor
