// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "sync/ISyncEngine.h"

#include "core/logging/VeorLogger.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// SyncEngineStub
// ─────────────────────────────────────────────────────────────────────────────
// No-op stub for sync engine. Full implementation deferred to future milestone.
// All operations log and return success without side effects.

class SyncEngineStub : public ISyncEngine {
 public:
  SyncEngineStub() = default;
  ~SyncEngineStub() override = default;

  Result<void, std::string> Initialize(const std::string& server_url) override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::Initialize — sync is disabled in this build");
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> Authenticate(const std::string& token) override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::Authenticate — no-op");
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> StartSync() override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::StartSync — no-op");
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> StopSync() override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::StopSync — no-op");
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> ForceSync() override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::ForceSync — no-op");
    return Result<void, std::string>::Ok();
  }

  SyncStatus GetStatus() const override {
    SyncStatus status;
    status.enabled = false;
    status.authenticated = false;
    status.error_message = "Sync not implemented in this build";
    return status;
  }

  bool IsSyncing() const override {
    return false;
  }

  Result<void, std::string> SetPassphrase(const std::string& passphrase) override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::SetPassphrase — no-op");
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> ClearData() override {
    VEOR_LOGI(LogCategory::kInfrastructure,
              "SyncEngineStub::ClearData — no-op");
    return Result<void, std::string>::Ok();
  }
};

}  // namespace veor
