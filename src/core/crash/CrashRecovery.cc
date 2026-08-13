// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/crash/CrashRecovery.h"

#include <fstream>

namespace veor {

CrashRecovery::CrashRecovery() = default;

Result<void, std::string> CrashRecovery::SaveSnapshot(
    const SessionSnapshot& snapshot) {
  // Simplified: real implementation uses JSON/binary serialization
  std::ofstream file(GetSnapshotPath(), std::ios::binary);
  if (!file.is_open()) {
    return Result<void, std::string>::Err("Cannot write snapshot");
  }
  // Write workspace count
  size_t count = snapshot.workspaces.size();
  file.write(reinterpret_cast<const char*>(&count), sizeof(count));
  for (const auto& ws : snapshot.workspaces) {
    size_t name_len = ws.name.size();
    file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
    file.write(ws.name.data(), name_len);
    size_t tab_count = ws.tab_urls.size();
    file.write(reinterpret_cast<const char*>(&tab_count), sizeof(tab_count));
    for (const auto& url : ws.tab_urls) {
      size_t url_len = url.size();
      file.write(reinterpret_cast<const char*>(&url_len), sizeof(url_len));
      file.write(url.data(), url_len);
    }
    file.write(reinterpret_cast<const char*>(&ws.active_tab_index),
               sizeof(ws.active_tab_index));
  }
  file.write(reinterpret_cast<const char*>(&snapshot.active_workspace_index),
             sizeof(snapshot.active_workspace_index));
  return Result<void, std::string>::Ok();
}

Result<SessionSnapshot, std::string> CrashRecovery::LoadSnapshot() {
  SessionSnapshot snapshot;
  std::ifstream file(GetSnapshotPath(), std::ios::binary);
  if (!file.is_open()) {
    return Result<SessionSnapshot, std::string>::Err("No snapshot found");
  }
  // Simplified read
  return Result<SessionSnapshot, std::string>::Ok(snapshot);
}

bool CrashRecovery::HasCrashedSession() const {
  std::ifstream snapshot(GetSnapshotPath());
  std::ifstream clean_exit(GetCleanExitPath());
  return snapshot.good() && !clean_exit.good();
}

Result<void, std::string> CrashRecovery::MarkCleanExit() {
  std::ofstream file(GetCleanExitPath());
  if (!file.is_open()) {
    return Result<void, std::string>::Err("Cannot mark clean exit");
  }
  file << "1";
  return Result<void, std::string>::Ok();
}

Result<void, std::string> CrashRecovery::RestoreSession(
    IWorkspaceManager* manager) {
  auto snapshot = LoadSnapshot();
  if (snapshot.IsErr()) {
    return Result<void, std::string>::Err(snapshot.UnwrapErr());
  }

  const auto& data = snapshot.Unwrap();
  for (const auto& ws_data : data.workspaces) {
    WorkspaceCreationOptions options;
    options.name = ws_data.name;
    options.activate_on_create = false;
    auto result = manager->CreateWorkspace(options);
    if (result.IsErr()) continue;

    // Restore tabs
    // for (const auto& url_str : ws_data.tab_urls) {
    //   GURL url(url_str);
    //   // Create tab in workspace
    // }
  }

  if (data.active_workspace_index >= 0 &&
      data.active_workspace_index < static_cast<int>(data.workspaces.size())) {
    // Activate workspace
  }

  return Result<void, std::string>::Ok();
}

std::string CrashRecovery::GetSnapshotPath() const {
  return "/tmp/veor_session_snapshot.bin";
}

std::string CrashRecovery::GetCleanExitPath() const {
  return "/tmp/veor_clean_exit.flag";
}

}  // namespace veor
