// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "infrastructure/platform/IPlatformService.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// PlatformServiceImpl
// ─────────────────────────────────────────────────────────────────────────────

class PlatformServiceImpl : public IPlatformService {
 public:
  PlatformServiceImpl();
  ~PlatformServiceImpl() override = default;

  // IPlatformService
  base::FilePath GetUserDataDirectory() const override;
  base::FilePath GetCacheDirectory() const override;
  base::FilePath GetDownloadsDirectory() const override;
  base::FilePath GetTempDirectory() const override;

  void ShowNotification(const std::string& title,
                        const std::string& body,
                        base::OnceCallback<void()> on_click) override;
  void SetBadgeCount(int count) override;
  void FlashTaskbar(bool flash) override;
  void SetAppUserModelId(const std::string& id) override;

  int GetLogicalCpuCount() const override;
  size_t GetTotalPhysicalMemory() const override;
};

}  // namespace veor
