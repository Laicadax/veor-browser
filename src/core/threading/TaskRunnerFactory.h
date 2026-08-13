// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "core/threading/ITaskRunner.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// TaskRunnerFactory
// ─────────────────────────────────────────────────────────────────────────────
// Creates task runners backed by Chromium's ThreadPool and UI thread.
//
// Thread safety: All methods are safe to call from any thread.

class TaskRunnerFactory : public ITaskRunnerFactory {
 public:
  TaskRunnerFactory();
  ~TaskRunnerFactory() override = default;

  std::unique_ptr<ITaskRunner> CreateSequenced(
      const std::string& name) override;
  std::unique_ptr<ITaskRunner> GetUiRunner() override;
  std::unique_ptr<ITaskRunner> GetIoRunner() override;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace veor
