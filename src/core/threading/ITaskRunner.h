// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// ITaskRunner
// ─────────────────────────────────────────────────────────────────────────────
// Abstraction over Chromium's task runners. Provides a simplified interface
// for posting tasks with delays.
//
// Thread safety: PostTask and PostDelayedTask are safe from any thread.
// RunsTasksInCurrentSequence is safe from any thread.

class ITaskRunner {
 public:
  virtual ~ITaskRunner() = default;

  // Posts a task to run asynchronously on this runner's sequence.
  // [Any Thread]
  virtual void PostTask(base::OnceClosure task) = 0;

  // Posts a delayed task. The task will not run before |delay| has elapsed.
  // [Any Thread]
  virtual void PostDelayedTask(base::OnceClosure task, base::TimeDelta delay) = 0;

  // Returns true if the current thread is the sequence owned by this runner.
  // [Any Thread]
  virtual bool RunsTasksInCurrentSequence() const = 0;

  // Returns the underlying Chromium task runner for interop with Chromium code.
  // [Any Thread]
  virtual scoped_refptr<base::SequencedTaskRunner> GetBaseRunner() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ITaskRunnerFactory
// ─────────────────────────────────────────────────────────────────────────────

class ITaskRunnerFactory {
 public:
  virtual ~ITaskRunnerFactory() = default;

  // Creates a new sequenced task runner for a named component.
  // The name is used for tracing and profiling.
  // [Any Thread]
  virtual std::unique_ptr<ITaskRunner> CreateSequenced(
      const std::string& name) = 0;

  // Returns the UI thread task runner.
  // [Any Thread]
  virtual std::unique_ptr<ITaskRunner> GetUiRunner() = 0;

  // Returns the I/O thread task runner (for blocking operations).
  // [Any Thread]
  virtual std::unique_ptr<ITaskRunner> GetIoRunner() = 0;
};

}  // namespace veor
