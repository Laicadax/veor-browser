// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// LogLevel
// ─────────────────────────────────────────────────────────────────────────────

enum class LogLevel {
  kVerbose,   // Development only, stripped in release builds
  kDebug,     // Detailed diagnostics
  kInfo,      // Normal operation
  kWarning,   // Recoverable issues
  kError,     // Operation failed
  kFatal      // Unrecoverable, terminates process after logging
};

// ─────────────────────────────────────────────────────────────────────────────
// LogCategory
// ─────────────────────────────────────────────────────────────────────────────

enum class LogCategory {
  kCore,
  kInfrastructure,
  kWorkspace,
  kTabs,
  kAnimation,
  kUI,
  kCommand,
  kExtensions,
  kSecurity,
  kSettings,
  kDownloads,
  kHistory,
  kBookmarks,
  kSync,
  kNetwork,
  kRenderer,
  kGeneral
};

// ─────────────────────────────────────────────────────────────────────────────
// LogEntry
// ─────────────────────────────────────────────────────────────────────────────
// Immutable log record. Passed by const reference to sinks.

struct LogEntry {
  base::TimeTicks timestamp;
  LogLevel level;
  LogCategory category;
  std::string message;
  const char* source_file = nullptr;
  int source_line = 0;
  uint64_t thread_id = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ILogSink
// ─────────────────────────────────────────────────────────────────────────────
// Interface for log output destinations (console, file, system log, etc.).
//
// Thread safety: Implementations must be thread-safe. Sinks run on a
// dedicated sequenced task runner to avoid blocking the caller.

class ILogSink {
 public:
  virtual ~ILogSink() = default;

  // Writes a log entry. Called on the sink's dedicated task runner.
  virtual void Write(const LogEntry& entry) = 0;

  // Returns true if this sink accepts entries at the given level.
  virtual bool AcceptsLevel(LogLevel level) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ILogger
// ─────────────────────────────────────────────────────────────────────────────
// Main logging interface. Thread-safe.
//
// Design:
//   - Hot path (Log) is lock-free: entries are enqueued to a ring buffer.
//   - A background thread drains the buffer and dispatches to sinks.
//   - Sinks run on their own sequenced task runners (no blocking).
//   - Verbose/Debug are compiled out in release unless --enable-debug-logs.

class ILogger {
 public:
  virtual ~ILogger() = default;

  // Logs a message. Thread-safe. May be called from any thread.
  //
  // |level|: severity of the message.
  // |category|: functional area.
  // |message|: the log text. Should not contain newlines.
  // |file|: source file path (__FILE__).
  // |line|: source line number (__LINE__).
  virtual void Log(LogLevel level,
                   LogCategory category,
                   const std::string& message,
                   const char* file,
                   int line) = 0;

  // Adds a sink. The logger takes ownership.
  virtual void AddSink(std::unique_ptr<ILogSink> sink) = 0;

  // Removes a sink. The sink pointer must match a previously added sink.
  virtual void RemoveSink(ILogSink* sink) = 0;

  // Sets the minimum level for all sinks. Entries below this level are
  // silently dropped (before enqueueing, for performance).
  virtual void SetMinLevel(LogLevel level) = 0;

  // Shuts down the logger. Flushes all pending entries to sinks.
  // Must be called before process exit.
  virtual void Shutdown() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Global accessor
// ─────────────────────────────────────────────────────────────────────────────
// Returns the process-global logger instance. Initialized during Core::Init.
// Never returns nullptr after initialization.

ILogger* GetLogger() noexcept;

// Sets the global logger instance. Called once during initialization.
void SetLogger(std::unique_ptr<ILogger> logger);

// ─────────────────────────────────────────────────────────────────────────────
// Convenience macros
// ─────────────────────────────────────────────────────────────────────────────
// These macros automatically capture __FILE__ and __LINE__.
// Verbose and Debug are no-ops in release builds unless debug logging is
// explicitly enabled at compile time.

#if defined(VEOR_ENABLE_VERBOSE_LOGGING)
#define VEOR_LOGV(cat, msg) \
  veor::GetLogger()->Log(veor::LogLevel::kVerbose, cat, msg, __FILE__, __LINE__)
#else
#define VEOR_LOGV(cat, msg) ((void)0)
#endif

#if defined(VEOR_ENABLE_DEBUG_LOGGING) && !defined(NDEBUG)
#define VEOR_LOGD(cat, msg) \
  veor::GetLogger()->Log(veor::LogLevel::kDebug, cat, msg, __FILE__, __LINE__)
#else
#define VEOR_LOGD(cat, msg) ((void)0)
#endif

#define VEOR_LOGI(cat, msg) \
  veor::GetLogger()->Log(veor::LogLevel::kInfo, cat, msg, __FILE__, __LINE__)

#define VEOR_LOGW(cat, msg) \
  veor::GetLogger()->Log(veor::LogLevel::kWarning, cat, msg, __FILE__, __LINE__)

#define VEOR_LOGE(cat, msg) \
  veor::GetLogger()->Log(veor::LogLevel::kError, cat, msg, __FILE__, __LINE__)

#define VEOR_LOGF(cat, msg) \
  veor::GetLogger()->Log(veor::LogLevel::kFatal, cat, msg, __FILE__, __LINE__)

}  // namespace veor
