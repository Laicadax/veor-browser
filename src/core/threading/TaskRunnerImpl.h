// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "core/threading/ITaskRunner.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// TaskRunnerImpl
// ─────────────────────────────────────────────────────────────────────────────
// Wraps a Chromium SequencedTaskRunner.

class TaskRunnerImpl : public ITaskRunner {
 public:
  explicit TaskRunnerImpl(scoped_refptr<base::SequencedTaskRunner> runner);
  ~TaskRunnerImpl() override = default;

  void PostTask(base::OnceClosure task) override;
  void PostDelayedTask(base::OnceClosure task, base::TimeDelta delay) override;
  bool RunsTasksInCurrentSequence() const override;
  scoped_refptr<base::SequencedTaskRunner> GetBaseRunner() const override;

 private:
  scoped_refptr<base::SequencedTaskRunner> runner_;
};

}  // namespace veor
