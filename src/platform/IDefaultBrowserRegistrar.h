// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "base/files/file_path.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// IDefaultBrowserRegistrar
// ─────────────────────────────────────────────────────────────────────────────
// Cross-platform interface for default browser registration.

class IDefaultBrowserRegistrar {
 public:
  virtual ~IDefaultBrowserRegistrar() = default;

  virtual bool IsDefault() const = 0;
  virtual bool Register() const = 0;
  virtual void OpenDefaultAppsSettings() const = 0;
  virtual std::string GetStatusMessage() const = 0;
};

// Factory function. Returns nullptr on unsupported platforms.
std::unique_ptr<IDefaultBrowserRegistrar> CreateDefaultBrowserRegistrar(
    const base::FilePath& executable_path);

}  // namespace veor
