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
// DownloadState
// ─────────────────────────────────────────────────────────────────────────────

enum class DownloadState {
  kPending,      // Queued, not started
  kInProgress,   // Actively downloading
  kPaused,       // User paused
  kCompleted,    // Finished successfully
  kFailed,       // Error occurred
  kCancelled,    // User cancelled
};

// ─────────────────────────────────────────────────────────────────────────────
// DownloadItem
// ─────────────────────────────────────────────────────────────────────────────

struct DownloadItem {
  DownloadId id;
  std::string url;
  std::string filename;
  std::string target_path;
  int64_t total_bytes = -1;
  int64_t received_bytes = 0;
  DownloadState state = DownloadState::kPending;
  int progress_percent = 0;
  std::string mime_type;
  bool can_resume = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// IDownloadController
// ─────────────────────────────────────────────────────────────────────────────

class IDownloadController {
 public:
  virtual ~IDownloadController() = default;

  // Lifecycle
  virtual Result<DownloadId, std::string> StartDownload(const std::string& url,
                                                         const std::string& suggested_filename) = 0;
  virtual Result<void, std::string> PauseDownload(DownloadId id) = 0;
  virtual Result<void, std::string> ResumeDownload(DownloadId id) = 0;
  virtual Result<void, std::string> CancelDownload(DownloadId id) = 0;

  // Queries
  virtual std::vector<DownloadItem> GetAllDownloads() const = 0;
  virtual const DownloadItem* GetDownload(DownloadId id) const = 0;

  // Sanitization
  virtual std::string SanitizeFilename(const std::string& filename) const = 0;
};

}  // namespace veor
