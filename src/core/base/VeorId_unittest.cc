// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "core/base/VeorId.h"

namespace veor {

TEST(VeorId, DefaultIsInvalid) {
  WorkspaceId id;
  EXPECT_FALSE(id.IsValid());
  EXPECT_EQ(id.value(), 0u);
}

TEST(VeorId, ExplicitValue) {
  WorkspaceId id(42);
  EXPECT_TRUE(id.IsValid());
  EXPECT_EQ(id.value(), 42u);
}

TEST(VeorId, DifferentTagsAreIncompatible) {
  WorkspaceId w(1);
  TabId t(1);
  EXPECT_EQ(w.value(), t.value());
  // Compile-time type safety prevents: w == t
  EXPECT_TRUE(w == WorkspaceId(1));
  EXPECT_TRUE(t == TabId(1));
}

TEST(VeorId, Comparison) {
  WorkspaceId a(1), b(2), c(1);
  EXPECT_TRUE(a < b);
  EXPECT_TRUE(a == c);
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(b > a);
  EXPECT_TRUE(a <= c);
  EXPECT_TRUE(b >= a);
}

TEST(VeorId, IdGenerator) {
  auto id1 = IdGenerator::NextId<WorkspaceTag>();
  auto id2 = IdGenerator::NextId<TabTag>();
  auto id3 = IdGenerator::NextId<WorkspaceTag>();

  EXPECT_TRUE(id1.IsValid());
  EXPECT_TRUE(id2.IsValid());
  EXPECT_NE(id1.value(), id2.value());
  EXPECT_NE(id2.value(), id3.value());
  EXPECT_EQ(id1.value() + 1, id3.value());  // Same generator, monotonic
}

TEST(VeorId, Hash) {
  WorkspaceId id(42);
  std::hash<WorkspaceId> hasher;
  EXPECT_EQ(hasher(id), hasher(WorkspaceId(42)));
  EXPECT_NE(hasher(id), hasher(WorkspaceId(43)));
}

TEST(VeorId, SentinelValues) {
  EXPECT_FALSE(kInvalidWorkspaceId.IsValid());
  EXPECT_FALSE(kInvalidTabId.IsValid());
  EXPECT_FALSE(kNoGroup.IsValid());
  EXPECT_FALSE(kInvalidExtensionId.IsValid());
}

}  // namespace veor
