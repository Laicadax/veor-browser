// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "core/memory/IMemoryTracker.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// MemoryTrackerImpl
// ─────────────────────────────────────────────────────────────────────────────
// Thread-safe memory tracking with per-component accounting.
//
// Design:
//   - Per-component allocations tracked in a mutex-protected map.
//   - Total and peak are atomic counters for lock-free reads.
//   - Pressure detection is checked on every allocation (fast path).

class MemoryTrackerImpl : public IMemoryTracker {
 public:
  MemoryTrackerImpl();
  ~MemoryTrackerImpl() override = default;

  void RecordAllocation(const std::string& component, size_t bytes) override;
  void RecordDeallocation(const std::string& component, size_t bytes) override;
  MemorySnapshot GetSnapshot() const override;
  void SetPressureThreshold(size_t bytes, PressureCallback callback) override;
  void ClearPressureThreshold() override;
  void RequestMemoryReduction(size_t target_bytes) override;

 private:
  void CheckPressure();

  struct ComponentStats {
    std::atomic<size_t> allocated{0};
    std::atomic<size_t> peak{0};
  };

  mutable std::mutex stats_mutex_;
  std::unordered_map<std::string, ComponentStats> stats_;

  std::atomic<size_t> total_allocated_{0};
  std::atomic<size_t> peak_allocated_{0};

  std::atomic<size_t> pressure_threshold_{0};
  PressureCallback pressure_callback_;
  std::mutex callback_mutex_;
  std::atomic<bool> pressure_fired_{false};
};

}  // namespace veor
