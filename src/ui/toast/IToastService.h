// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>

#include "base/functional/callback.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// IToastService
// ─────────────────────────────────────────────────────────────────────────────
// Bottom-right notifications. Thin border. Progress bar.
// Auto-dismiss with countdown. No icons. Text only.
// ─────────────────────────────────────────────────────────────────────────────

struct ToastData {
  std::string title;
  std::string detail;
  int duration_ms = 4000;
  base::RepeatingClosure action;
  std::string action_label;
};

class IToastService {
 public:
  virtual ~IToastService() = default;

  virtual void Show(const ToastData& toast) = 0;
  virtual void DismissAll() = 0;
};

}  // namespace veor
