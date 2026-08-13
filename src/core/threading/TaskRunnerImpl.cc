// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/threading/TaskRunnerImpl.h"

namespace veor {

TaskRunnerImpl::TaskRunnerImpl(scoped_refptr<base::SequencedTaskRunner> runner)
    : runner_(std::move(runner)) {}

void TaskRunnerImpl::PostTask(base::OnceClosure task) {
  runner_->PostTask(FROM_HERE, std::move(task));
}

void TaskRunnerImpl::PostDelayedTask(base::OnceClosure task,
                                      base::TimeDelta delay) {
  runner_->PostDelayedTask(FROM_HERE, std::move(task), delay);
}

bool TaskRunnerImpl::RunsTasksInCurrentSequence() const {
  return runner_->RunsTasksInCurrentSequence();
}

scoped_refptr<base::SequencedTaskRunner> TaskRunnerImpl::GetBaseRunner() const {
  return runner_;
}

}  // namespace veor
