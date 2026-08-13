// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/CoreInit.h"

#include <atomic>
#include <memory>

#include "core/config/ConfigProviderImpl.h"
#include "core/logging/VeorLogger.h"
#include "core/memory/MemoryTrackerImpl.h"
#include "core/threading/TaskRunnerFactory.h"

namespace veor {

namespace {

struct CoreState {
  std::unique_ptr<ILogger> logger;
  std::unique_ptr<ITaskRunnerFactory> task_runner_factory;
  std::unique_ptr<IConfigProvider> config_provider;
  std::unique_ptr<IMemoryTracker> memory_tracker;
};

std::atomic<CoreState*> g_core_state{nullptr};

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// CoreInit
// ─────────────────────────────────────────────────────────────────────────────

Result<void, std::string> CoreInit::Initialize(const CoreInitOptions& options) {
  if (IsInitialized()) {
    return Result<void, std::string>::Err("Core already initialized");
  }

  auto state = std::make_unique<CoreState>();

  // 1. Logger
  state->logger = CreateDefaultLogger(options.log_file_path,
                                      options.min_log_level);
  SetLogger(std::move(state->logger));

  VEOR_LOGI(LogCategory::kCore, "Core initialization starting...");

  // 2. TaskRunnerFactory
  state->task_runner_factory = std::make_unique<TaskRunnerFactory>();

  // 3. ConfigProvider
  auto config = std::make_unique<ConfigProviderImpl>(
      state->task_runner_factory->GetIoRunner());
  if (!options.config_file_path.empty()) {
    config->SetFilePath(options.config_file_path);
    auto load_result = config->Load();
    if (load_result.IsErr()) {
      VEOR_LOGW(LogCategory::kCore,
                "Config load failed: " + load_result.UnwrapErr());
    }
  }
  state->config_provider = std::move(config);

  // 4. MemoryTracker
  state->memory_tracker = std::make_unique<MemoryTrackerImpl>();
  if (options.memory_pressure_threshold > 0) {
    state->memory_tracker->SetPressureThreshold(
        options.memory_pressure_threshold,
        base::BindRepeating([](MemorySnapshot snapshot) {
          VEOR_LOGW(LogCategory::kCore,
                    "Memory pressure: " +
                        std::to_string(snapshot.total_allocated_bytes) +
                        " bytes");
        }));
  }

  g_core_state.store(state.release(), std::memory_order_release);
  VEOR_LOGI(LogCategory::kCore, "Core initialization complete");
  return Result<void, std::string>::Ok();
}

void CoreInit::Shutdown() {
  CoreState* state = g_core_state.exchange(nullptr, std::memory_order_acq_rel);
  if (!state)
    return;

  VEOR_LOGI(LogCategory::kCore, "Core shutdown starting...");

  if (state->config_provider) {
    auto save_result = state->config_provider->SaveNow();
    if (save_result.IsErr()) {
      VEOR_LOGW(LogCategory::kCore,
                "Config save failed during shutdown: " + save_result.UnwrapErr());
    }
  }

  if (state->logger) {
    state->logger->Shutdown();
  }

  delete state;
}

bool CoreInit::IsInitialized() {
  return g_core_state.load(std::memory_order_acquire) != nullptr;
}

ILogger* CoreInit::GetLogger() {
  auto* state = g_core_state.load(std::memory_order_acquire);
  return state ? state->logger.get() : nullptr;
}

ITaskRunnerFactory* CoreInit::GetTaskRunnerFactory() {
  auto* state = g_core_state.load(std::memory_order_acquire);
  return state ? state->task_runner_factory.get() : nullptr;
}

IConfigProvider* CoreInit::GetConfigProvider() {
  auto* state = g_core_state.load(std::memory_order_acquire);
  return state ? state->config_provider.get() : nullptr;
}

IMemoryTracker* CoreInit::GetMemoryTracker() {
  auto* state = g_core_state.load(std::memory_order_acquire);
  return state ? state->memory_tracker.get() : nullptr;
}

}  // namespace veor
