// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstddef>
#include <string>

#include "base/functional/callback.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// MemorySnapshot
// ─────────────────────────────────────────────────────────────────────────────

struct MemorySnapshot {
  size_t total_allocated_bytes = 0;
  size_t peak_allocated_bytes = 0;
  size_t tab_count = 0;
  size_t workspace_count = 0;
  size_t renderer_process_count = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IMemoryTracker
// ─────────────────────────────────────────────────────────────────────────────
// Tracks memory allocations by component and provides pressure detection.
//
// Thread safety: All methods are thread-safe.

class IMemoryTracker {
 public:
  virtual ~IMemoryTracker() = default;

  // Records an allocation attributed to a component.
  // [Any Thread]
  virtual void RecordAllocation(const std::string& component, size_t bytes) = 0;

  // Records a deallocation.
  // [Any Thread]
  virtual void RecordDeallocation(const std::string& component, size_t bytes) = 0;

  // Returns the current memory snapshot.
  // [Any Thread]
  virtual MemorySnapshot GetSnapshot() const = 0;

  // Registers a callback invoked when total memory exceeds the threshold.
  // Only one callback may be registered at a time.
  // [Any Thread]
  using PressureCallback = base::RepeatingCallback<void(MemorySnapshot)>;
  virtual void SetPressureThreshold(size_t bytes, PressureCallback callback) = 0;

  // Clears the pressure threshold.
  // [Any Thread]
  virtual void ClearPressureThreshold() = 0;

  // Requests memory reduction. The implementation decides which components
  // to target (e.g., sleeping tabs, clearing caches).
  // [Any Thread]
  virtual void RequestMemoryReduction(size_t target_bytes) = 0;
};

}  // namespace veor
