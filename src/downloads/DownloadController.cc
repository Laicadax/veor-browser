// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "downloads/DownloadController.h"

#include <algorithm>
#include <cctype>

#include "core/base/VeorId.h"

namespace veor {

DownloadController::DownloadController() = default;

Result<DownloadId, std::string> DownloadController::StartDownload(
    const std::string& url,
    const std::string& suggested_filename) {
  DownloadId id = IdGenerator::NextId<DownloadTag>();

  DownloadItem item;
  item.id = id;
  item.url = url;
  item.filename = SanitizeFilename(suggested_filename);
  item.target_path = "/downloads/" + item.filename;
  item.state = DownloadState::kInProgress;
  item.total_bytes = 1024 * 1024;  // Placeholder

  downloads_[id] = std::move(item);
  return Result<DownloadId, std::string>::Ok(id);
}

Result<void, std::string> DownloadController::PauseDownload(DownloadId id) {
  auto it = downloads_.find(id);
  if (it == downloads_.end()) {
    return Result<void, std::string>::Err("Download not found");
  }
  if (it->second.state == DownloadState::kInProgress) {
    it->second.state = DownloadState::kPaused;
    it->second.can_resume = true;
  }
  return Result<void, std::string>::Ok();
}

Result<void, std::string> DownloadController::ResumeDownload(DownloadId id) {
  auto it = downloads_.find(id);
  if (it == downloads_.end()) {
    return Result<void, std::string>::Err("Download not found");
  }
  if (it->second.state == DownloadState::kPaused && it->second.can_resume) {
    it->second.state = DownloadState::kInProgress;
  }
  return Result<void, std::string>::Ok();
}

Result<void, std::string> DownloadController::CancelDownload(DownloadId id) {
  auto it = downloads_.find(id);
  if (it == downloads_.end()) {
    return Result<void, std::string>::Err("Download not found");
  }
  it->second.state = DownloadState::kCancelled;
  return Result<void, std::string>::Ok();
}

std::vector<DownloadItem> DownloadController::GetAllDownloads() const {
  std::vector<DownloadItem> result;
  for (const auto& [id, item] : downloads_) {
    result.push_back(item);
  }
  return result;
}

const DownloadItem* DownloadController::GetDownload(DownloadId id) const {
  auto it = downloads_.find(id);
  return it != downloads_.end() ? &it->second : nullptr;
}

std::string DownloadController::SanitizeFilename(const std::string& filename) const {
  std::string sanitized;
  sanitized.reserve(filename.size());
  for (char c : filename) {
    if (std::isalnum(c) || c == '.' || c == '-' || c == '_') {
      sanitized.push_back(c);
    } else {
      sanitized.push_back('_');
    }
  }
  if (sanitized.empty() || sanitized == ".") {
    sanitized = "download";
  }
  return sanitized;
}

}  // namespace veor
