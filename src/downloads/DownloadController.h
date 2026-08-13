// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unordered_map>

#include "downloads/IDownloadController.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// DownloadController
// ─────────────────────────────────────────────────────────────────────────────

class DownloadController : public IDownloadController {
 public:
  DownloadController();
  ~DownloadController() override = default;

  // IDownloadController
  Result<DownloadId, std::string> StartDownload(
      const std::string& url,
      const std::string& suggested_filename) override;
  Result<void, std::string> PauseDownload(DownloadId id) override;
  Result<void, std::string> ResumeDownload(DownloadId id) override;
  Result<void, std::string> CancelDownload(DownloadId id) override;

  std::vector<DownloadItem> GetAllDownloads() const override;
  const DownloadItem* GetDownload(DownloadId id) const override;

  std::string SanitizeFilename(const std::string& filename) const override;

 private:
  std::unordered_map<DownloadId, DownloadItem, DownloadId::Hash> downloads_;
};

}  // namespace veor
