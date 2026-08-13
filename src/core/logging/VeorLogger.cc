// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/logging/VeorLogger.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"

namespace veor {

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// LogLevel helpers
// ─────────────────────────────────────────────────────────────────────────────

const char* LogLevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::kVerbose:  return "VERBOSE";
    case LogLevel::kDebug:    return "DEBUG  ";
    case LogLevel::kInfo:     return "INFO   ";
    case LogLevel::kWarning:  return "WARNING";
    case LogLevel::kError:    return "ERROR  ";
    case LogLevel::kFatal:    return "FATAL  ";
  }
  return "UNKNOWN";
}

const char* LogCategoryToString(LogCategory category) {
  switch (category) {
    case LogCategory::kCore:           return "CORE";
    case LogCategory::kInfrastructure: return "INFRA";
    case LogCategory::kWorkspace:      return "WRK";
    case LogCategory::kTabs:           return "TAB";
    case LogCategory::kAnimation:      return "ANI";
    case LogCategory::kUI:             return "UI";
    case LogCategory::kCommand:        return "CMD";
    case LogCategory::kExtensions:     return "EXT";
    case LogCategory::kSecurity:       return "SEC";
    case LogCategory::kSettings:        return "SET";
    case LogCategory::kDownloads:      return "DL";
    case LogCategory::kHistory:        return "HIST";
    case LogCategory::kBookmarks:      return "BM";
    case LogCategory::kSync:           return "SYNC";
    case LogCategory::kNetwork:        return "NET";
    case LogCategory::kRenderer:       return "RNDR";
    case LogCategory::kGeneral:        return "GEN";
  }
  return "???";
}

// ─────────────────────────────────────────────────────────────────────────────
// ConsoleLogSink
// ─────────────────────────────────────────────────────────────────────────────

class ConsoleLogSink : public ILogSink {
 public:
  explicit ConsoleLogSink(LogLevel min_level) : min_level_(min_level) {}

  void Write(const LogEntry& entry) override {
    // Simple console output. In production, this could use OutputDebugString
    // on Windows or syslog on Linux.
    std::cerr << base::StringPrintf(
        "[%s] [%s] [%s] %s:%d | %s\n",
        base::TimeFormatAsIso8601(entry.timestamp).c_str(),
        LogLevelToString(entry.level),
        LogCategoryToString(entry.category),
        entry.source_file ? entry.source_file : "?",
        entry.source_line,
        entry.message.c_str());
  }

  bool AcceptsLevel(LogLevel level) const override {
    return static_cast<int>(level) >= static_cast<int>(min_level_);
  }

 private:
  LogLevel min_level_;
};

// ─────────────────────────────────────────────────────────────────────────────
// FileLogSink
// ─────────────────────────────────────────────────────────────────────────────

class FileLogSink : public ILogSink {
 public:
  explicit FileLogSink(const std::string& path, LogLevel min_level)
      : min_level_(min_level), file_path_(path) {
    file_.open(path, std::ios::out | std::ios::app);
  }

  ~FileLogSink() override {
    if (file_.is_open()) {
      file_.close();
    }
  }

  void Write(const LogEntry& entry) override {
    if (!file_.is_open())
      return;

    file_ << base::StringPrintf(
        "[%s] [%s] [%s] %s:%d | %s\n",
        base::TimeFormatAsIso8601(entry.timestamp).c_str(),
        LogLevelToString(entry.level),
        LogCategoryToString(entry.category),
        entry.source_file ? entry.source_file : "?",
        entry.source_line,
        entry.message.c_str());
    file_.flush();
  }

  bool AcceptsLevel(LogLevel level) const override {
    return static_cast<int>(level) >= static_cast<int>(min_level_);
  }

 private:
  LogLevel min_level_;
  std::string file_path_;
  std::ofstream file_;
};

// ─────────────────────────────────────────────────────────────────────────────
// VeorLogger (ILogger implementation)
// ─────────────────────────────────────────────────────────────────────────────
// Thread-safe logger with a background drain thread.
//
// Design:
//   - Log() enqueues entries to a mutex-protected queue.
//   - A background thread drains the queue and dispatches to sinks.
//   - Each sink runs on the background thread (sinks are assumed to be fast).
//   - For slow sinks (file I/O), the sink implementation should offload to
//     its own task runner.

class VeorLogger : public ILogger {
 public:
  VeorLogger() : min_level_(LogLevel::kInfo), shutdown_(false) {
    drain_thread_ = std::thread(&VeorLogger::DrainLoop, this);
  }

  ~VeorLogger() override {
    Shutdown();
  }

  void Log(LogLevel level,
           LogCategory category,
           const std::string& message,
           const char* file,
           int line) override {
    if (static_cast<int>(level) < static_cast<int>(min_level_.load()))
      return;

    LogEntry entry;
    entry.timestamp = base::TimeTicks::Now();
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.source_file = file;
    entry.source_line = line;
    entry.thread_id = reinterpret_cast<uint64_t>(
        base::PlatformThread::CurrentId());

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      queue_.push(std::move(entry));
    }
    queue_cv_.notify_one();

    // Fatal logs are synchronous: flush immediately.
    if (level == LogLevel::kFatal) {
      Flush();
    }
  }

  void AddSink(std::unique_ptr<ILogSink> sink) override {
    std::lock_guard<std::mutex> lock(sinks_mutex_);
    sinks_.push_back(std::move(sink));
  }

  void RemoveSink(ILogSink* sink) override {
    std::lock_guard<std::mutex> lock(sinks_mutex_);
    sinks_.erase(
        std::remove_if(sinks_.begin(), sinks_.end(),
                       [sink](const std::unique_ptr<ILogSink>& ptr) {
                         return ptr.get() == sink;
                       }),
        sinks_.end());
  }

  void SetMinLevel(LogLevel level) override {
    min_level_.store(level);
  }

  void Shutdown() override {
    bool expected = false;
    if (!shutdown_.compare_exchange_strong(expected, true))
      return;  // Already shut down

    queue_cv_.notify_all();
    if (drain_thread_.joinable()) {
      drain_thread_.join();
    }
    Flush();  // Drain remaining entries
  }

 private:
  void DrainLoop() {
    while (!shutdown_.load()) {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_cv_.wait(lock, [this] { return !queue_.empty() || shutdown_.load(); });

      // Move all entries out of the queue to minimize lock hold time.
      std::queue<LogEntry> local_queue;
      local_queue.swap(queue_);
      lock.unlock();

      while (!local_queue.empty()) {
        DispatchToSinks(local_queue.front());
        local_queue.pop();
      }
    }
  }

  void Flush() {
    std::queue<LogEntry> local_queue;
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      local_queue.swap(queue_);
    }

    while (!local_queue.empty()) {
      DispatchToSinks(local_queue.front());
      local_queue.pop();
    }
  }

  void DispatchToSinks(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(sinks_mutex_);
    for (const auto& sink : sinks_) {
      if (sink->AcceptsLevel(entry.level)) {
        sink->Write(entry);
      }
    }
  }

  std::atomic<LogLevel> min_level_;
  std::atomic<bool> shutdown_;

  // Queue
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::queue<LogEntry> queue_;

  // Sinks
  std::mutex sinks_mutex_;
  std::vector<std::unique_ptr<ILogSink>> sinks_;

  std::thread drain_thread_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Global instance
// ─────────────────────────────────────────────────────────────────────────────

std::atomic<ILogger*> g_logger{nullptr};

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

ILogger* GetLogger() noexcept {
  ILogger* logger = g_logger.load(std::memory_order_acquire);
  if (!logger) {
    // Fallback: create a basic console logger. This should only happen if
    // Core::Init was not called.
    static std::unique_ptr<VeorLogger> fallback_logger;
    static std::once_flag fallback_flag;
    std::call_once(fallback_flag, []() {
      fallback_logger = std::make_unique<VeorLogger>();
      fallback_logger->AddSink(
          std::make_unique<ConsoleLogSink>(LogLevel::kWarning));
      g_logger.store(fallback_logger.get(), std::memory_order_release);
    });
    logger = fallback_logger.get();
  }
  return logger;
}

void SetLogger(std::unique_ptr<ILogger> logger) {
  // The old logger is leaked here by design; we never destroy the global
  // logger after SetLogger to avoid use-after-free during shutdown.
  // In practice, this is called once during Core::Init.
  g_logger.store(logger.release(), std::memory_order_release);
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory helpers
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<ILogger> CreateDefaultLogger(const std::string& log_file_path,
                                              LogLevel min_level) {
  auto logger = std::make_unique<VeorLogger>();
  logger->AddSink(std::make_unique<ConsoleLogSink>(min_level));
  if (!log_file_path.empty()) {
    logger->AddSink(std::make_unique<FileLogSink>(log_file_path, min_level));
  }
  return logger;
}

}  // namespace veor
