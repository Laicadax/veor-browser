// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "core/memory/MemoryTrackerImpl.h"
#include "core/test/VeorTestBase.h"

namespace veor {

class MemoryTrackerTest : public VeorTestBase {};

TEST_F(MemoryTrackerTest, AllocationTracking) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 1024);
  tracker.RecordAllocation("tabs", 512);

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 1536u);
  EXPECT_EQ(snap.peak_allocated_bytes, 1536u);
  EXPECT_EQ(snap.tab_count, 1u);
}

TEST_F(MemoryTrackerTest, DeallocationTracking) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 1024);
  tracker.RecordDeallocation("tabs", 512);

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 512u);
  EXPECT_EQ(snap.peak_allocated_bytes, 1024u);
}

TEST_F(MemoryTrackerTest, FullDeallocation) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 1000);
  tracker.RecordDeallocation("tabs", 1000);

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 0u);
  EXPECT_EQ(snap.peak_allocated_bytes, 1000u);
}

TEST_F(MemoryTrackerTest, OverDeallocation) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 100);
  tracker.RecordDeallocation("tabs", 200);  // More than allocated

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 0u);  // Clamped to zero
}

TEST_F(MemoryTrackerTest, MultipleComponents) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 1000);
  tracker.RecordAllocation("workspaces", 500);
  tracker.RecordAllocation("renderer", 2000);

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 3500u);
  EXPECT_EQ(snap.tab_count, 1u);
  EXPECT_EQ(snap.workspace_count, 1u);
  EXPECT_EQ(snap.renderer_process_count, 1u);
}

TEST_F(MemoryTrackerTest, PeakTracking) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 1000);
  tracker.RecordDeallocation("tabs", 500);
  tracker.RecordAllocation("tabs", 200);

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 700u);
  EXPECT_EQ(snap.peak_allocated_bytes, 1000u);
}

TEST_F(MemoryTrackerTest, PressureCallback) {
  MemoryTrackerImpl tracker;
  bool fired = false;
  MemorySnapshot captured;

  tracker.SetPressureThreshold(500, base::BindRepeating(
      [&fired, &captured](MemorySnapshot s) {
        fired = true;
        captured = s;
      }));

  tracker.RecordAllocation("tabs", 1000);
  EXPECT_TRUE(fired);
  EXPECT_EQ(captured.total_allocated_bytes, 1000u);
}

TEST_F(MemoryTrackerTest, PressureCallbackFiresOnce) {
  MemoryTrackerImpl tracker;
  int count = 0;

  tracker.SetPressureThreshold(100, base::BindRepeating(
      [&count](MemorySnapshot) { count++; }));

  tracker.RecordAllocation("a", 200);
  tracker.RecordAllocation("b", 200);
  EXPECT_EQ(count, 1);  // Should fire only once until cleared
}

TEST_F(MemoryTrackerTest, ClearPressureThreshold) {
  MemoryTrackerImpl tracker;
  bool fired = false;

  tracker.SetPressureThreshold(100, base::BindRepeating(
      [&fired](MemorySnapshot) { fired = true; }));
  tracker.ClearPressureThreshold();

  tracker.RecordAllocation("tabs", 1000);
  EXPECT_FALSE(fired);
}

TEST_F(MemoryTrackerTest, UnknownDeallocation) {
  MemoryTrackerImpl tracker;
  // Should not crash, just log a warning
  tracker.RecordDeallocation("unknown_component", 100);

  auto snap = tracker.GetSnapshot();
  EXPECT_EQ(snap.total_allocated_bytes, 0u);
}

TEST_F(MemoryTrackerTest, RequestMemoryReduction) {
  MemoryTrackerImpl tracker;
  tracker.RecordAllocation("tabs", 10000);

  // Should not crash
  tracker.RequestMemoryReduction(5000);

  auto snap = tracker.GetSnapshot();
  // Placeholder: reduction is not yet implemented
  EXPECT_EQ(snap.total_allocated_bytes, 10000u);
}

}  // namespace veor
