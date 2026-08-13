// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/threading/TaskRunnerFactory.h"

#include "core/threading/TaskRunnerImpl.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"

namespace veor {

class TaskRunnerFactory::Impl {
 public:
  scoped_refptr<base::SequencedTaskRunner> GetUiRunner() {
    // The UI runner is the current sequence at the time of first request.
    // This should be called from the UI thread during initialization.
    if (!ui_runner_) {
      ui_runner_ = base::SequencedTaskRunner::GetCurrentDefault();
    }
    return ui_runner_;
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> ui_runner_;
};

TaskRunnerFactory::TaskRunnerFactory()
    : impl_(std::make_unique<Impl>()) {}

std::unique_ptr<ITaskRunner> TaskRunnerFactory::CreateSequenced(
    const std::string& name) {
  return std::make_unique<TaskRunnerImpl>(
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::TaskPriority::USER_VISIBLE, base::MayBlock(),
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}));
}

std::unique_ptr<ITaskRunner> TaskRunnerFactory::GetUiRunner() {
  return std::make_unique<TaskRunnerImpl>(impl_->GetUiRunner());
}

std::unique_ptr<ITaskRunner> TaskRunnerFactory::GetIoRunner() {
  return std::make_unique<TaskRunnerImpl>(
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::TaskPriority::BEST_EFFORT, base::MayBlock(),
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN}));
}

}  // namespace veor
