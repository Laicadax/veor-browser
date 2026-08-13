// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "core/base/VeorResult.h"

namespace veor {

enum class SyncDataType {
  kBookmarks,
  kHistory,
  kPasswords,
  kAutofill,
  kSettings,
  kTabs,
  kExtensions,
};

struct SyncStatus {
  bool enabled = false;
  bool authenticated = false;
  std::string last_sync_time;
  size_t pending_changes = 0;
  std::string error_message;
};

// ─────────────────────────────────────────────────────────────────────────────
// ISyncEngine
// ─────────────────────────────────────────────────────────────────────────────
// Cross-device synchronization interface.
// Stub implementation for future self-hosted Chromium Sync Server.

class ISyncEngine {
 public:
  virtual ~ISyncEngine() = default;

  virtual Result<void, std::string> Initialize(const std::string& server_url) = 0;
  virtual Result<void, std::string> Authenticate(const std::string& token) = 0;
  virtual Result<void, std::string> StartSync() = 0;
  virtual Result<void, std::string> StopSync() = 0;
  virtual Result<void, std::string> ForceSync() = 0;

  virtual SyncStatus GetStatus() const = 0;
  virtual bool IsSyncing() const = 0;

  virtual Result<void, std::string> SetPassphrase(const std::string& passphrase) = 0;
  virtual Result<void, std::string> ClearData() = 0;
};

}  // namespace veor
