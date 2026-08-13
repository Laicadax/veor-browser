// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// IPlatformService
// ─────────────────────────────────────────────────────────────────────────────
// OS-level integration: paths, notifications, hardware info.
//
// Thread safety:
//   - Path queries: [Any Thread]
//   - UI methods (notifications, flash): [UI Thread]

class IPlatformService {
 public:
  virtual ~IPlatformService() = default;

  // ── Paths ──
  virtual base::FilePath GetUserDataDirectory() const = 0;
  virtual base::FilePath GetCacheDirectory() const = 0;
  virtual base::FilePath GetDownloadsDirectory() const = 0;
  virtual base::FilePath GetTempDirectory() const = 0;

  // ── Notifications ──
  virtual void ShowNotification(const std::string& title,
                                const std::string& body,
                                base::OnceCallback<void()> on_click) = 0;

  // ── OS Integration ──
  virtual void SetBadgeCount(int count) = 0;
  virtual void FlashTaskbar(bool flash) = 0;
  virtual void SetAppUserModelId(const std::string& id) = 0;

  // ── Hardware ──
  virtual int GetLogicalCpuCount() const = 0;
  virtual size_t GetTotalPhysicalMemory() const = 0;
};

}  // namespace veor
