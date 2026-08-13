// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "core/base/VeorResult.h"

#include <memory>
#include <string>

namespace veor {

// Forward declarations
class ILogger;
class ITaskRunnerFactory;
class IConfigProvider;
class IMemoryTracker;

// ─────────────────────────────────────────────────────────────────────────────
// CoreInit
// ─────────────────────────────────────────────────────────────────────────────
// Initializes all Core layer components. Must be called once during browser
// startup, before any other VEOR code is used.
//
// Thread safety: Not thread-safe. Must be called from the main thread.

struct CoreInitOptions {
  // Path to the log file. Empty = no file logging.
  std::string log_file_path;

  // Minimum log level for the default logger.
  LogLevel min_log_level = LogLevel::kInfo;

  // Path to the config file. Empty = in-memory config only.
  std::string config_file_path;

  // Memory pressure threshold in bytes. 0 = disabled.
  size_t memory_pressure_threshold = 0;
};

class CoreInit {
 public:
  // Initializes the Core layer. Returns error if initialization fails.
  static Result<void, std::string> Initialize(const CoreInitOptions& options);

  // Shuts down the Core layer. Flushes logs, saves config, releases resources.
  static void Shutdown();

  // Returns true if Core is initialized.
  static bool IsInitialized();

  // Accessors for initialized components. nullptr if not initialized.
  static ILogger* GetLogger();
  static ITaskRunnerFactory* GetTaskRunnerFactory();
  static IConfigProvider* GetConfigProvider();
  static IMemoryTracker* GetMemoryTracker();

 private:
  CoreInit() = delete;
};

}  // namespace veor
