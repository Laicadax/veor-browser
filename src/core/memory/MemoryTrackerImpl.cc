// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/memory/MemoryTrackerImpl.h"
#include "workspace/Workspace.h"

#include <algorithm>

#include "core/logging/VeorLogger.h"

namespace veor {

MemoryTrackerImpl::MemoryTrackerImpl() = default;

void MemoryTrackerImpl::RecordAllocation(const std::string& component,
                                         size_t bytes) {
  if (bytes == 0)
    return;

  ComponentStats* stats = nullptr;
  {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats = &stats_[component];
  }

  size_t new_allocated = stats->allocated.fetch_add(bytes, std::memory_order_relaxed) + bytes;

  // Update peak
  size_t expected_peak = stats->peak.load(std::memory_order_relaxed);
  while (new_allocated > expected_peak &&
         !stats->peak.compare_exchange_weak(expected_peak, new_allocated,
                                            std::memory_order_relaxed)) {
    // Retry
  }

  size_t new_total = total_allocated_.fetch_add(bytes, std::memory_order_relaxed) + bytes;

  // Update global peak
  size_t expected_global_peak = peak_allocated_.load(std::memory_order_relaxed);
  while (new_total > expected_global_peak &&
         !peak_allocated_.compare_exchange_weak(expected_global_peak, new_total,
                                                 std::memory_order_relaxed)) {
    // Retry
  }

  CheckPressure();
}

void MemoryTrackerImpl::RecordDeallocation(const std::string& component,
                                           size_t bytes) {
  if (bytes == 0)
    return;

  ComponentStats* stats = nullptr;
  {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    auto it = stats_.find(component);
    if (it == stats_.end()) {
      VEOR_LOGW(LogCategory::kCore,
                "Deallocation for unknown component: " + component);
      return;
    }
    stats = &it->second;
  }

  size_t old = stats->allocated.fetch_sub(bytes, std::memory_order_relaxed);
  if (old < bytes) {
    // Underflow: more deallocated than allocated. Reset to zero.
    stats->allocated.store(0, std::memory_order_relaxed);
    total_allocated_.fetch_sub(old, std::memory_order_relaxed);
  } else {
    total_allocated_.fetch_sub(bytes, std::memory_order_relaxed);
  }
}

MemorySnapshot MemoryTrackerImpl::GetSnapshot() const {
  MemorySnapshot snapshot;
  snapshot.total_allocated_bytes = total_allocated_.load(std::memory_order_relaxed);
  snapshot.peak_allocated_bytes = peak_allocated_.load(std::memory_order_relaxed);

  std::lock_guard<std::mutex> lock(stats_mutex_);
  snapshot.tab_count = 0;
  snapshot.workspace_count = 0;
  snapshot.renderer_process_count = 0;

  for (const auto& [name, stats] : stats_) {
    if (name.find("tab") != std::string::npos) {
      snapshot.tab_count++;
    } else if (name.find("workspace") != std::string::npos) {
      snapshot.workspace_count++;
    } else if (name.find("renderer") != std::string::npos) {
      snapshot.renderer_process_count++;
    }
  }

  return snapshot;
}

void MemoryTrackerImpl::SetPressureThreshold(size_t bytes,
                                             PressureCallback callback) {
  pressure_threshold_.store(bytes, std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    pressure_callback_ = std::move(callback);
  }
  pressure_fired_.store(false, std::memory_order_relaxed);
}

void MemoryTrackerImpl::ClearPressureThreshold() {
  pressure_threshold_.store(0, std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    pressure_callback_.Reset();
  }
  pressure_fired_.store(false, std::memory_order_relaxed);
}

void MemoryTrackerImpl::RequestMemoryReduction(size_t target_bytes) {
  VEOR_LOGI(LogCategory::kCore,
            "Memory reduction requested: target " + std::to_string(target_bytes) +
                " bytes");

  auto snapshot = GetSnapshot();
  VEOR_LOGI(LogCategory::kCore,
            "Current memory: " + std::to_string(snapshot.total_allocated_bytes) +
                " bytes, peak: " + std::to_string(snapshot.peak_allocated_bytes) +
                ", tabs: " + std::to_string(snapshot.tab_count));

  if (snapshot.tab_count > 1) {
    VEOR_LOGI(LogCategory::kCore,
              "Memory reduction: consider sleeping background tabs (" +
                  std::to_string(snapshot.tab_count - 1) + " inactive)");
  }

  VEOR_LOGI(LogCategory::kCore,
            "Memory reduction: image caches, font caches, and renderer "
            "trimming delegated to Chromium content layer");
}

void MemoryTrackerImpl::CheckPressure() {
  size_t threshold = pressure_threshold_.load(std::memory_order_relaxed);
  if (threshold == 0)
    return;

  size_t current = total_allocated_.load(std::memory_order_relaxed);
  if (current < threshold)
    return;

  bool expected = false;
  if (!pressure_fired_.compare_exchange_strong(expected, true,
                                                 std::memory_order_relaxed)) {
    return;  // Already fired.
  }

  PressureCallback callback;
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callback = pressure_callback_;
  }

  if (callback) {
    VEOR_LOGW(LogCategory::kCore,
              "Memory pressure detected: " + std::to_string(current) +
                  " bytes (threshold: " + std::to_string(threshold) + ")");
    callback.Run(GetSnapshot());
  }
}

}  // namespace veor
