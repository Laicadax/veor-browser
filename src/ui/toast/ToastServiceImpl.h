// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "ui/toast/IToastService.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;

// ─────────────────────────────────────────────────────────────────────────────
// ToastServiceImpl
// ─────────────────────────────────────────────────────────────────────────────
// Manages a vertical stack of toast views at bottom-right.
// Each toast: thin border, title, detail, progress bar, optional action.
// ─────────────────────────────────────────────────────────────────────────────

class ToastServiceImpl : public IToastService {
 public:
  explicit ToastServiceImpl(IThemeProvider* theme);
  ~ToastServiceImpl() override;

  void Show(const ToastData& toast) override;
  void DismissAll() override;

  // Attach to a parent view (BrowserShell's overlay layer)
  void SetContainer(views::View* container);

 private:
  struct ActiveToast {
    int64_t id = 0;
    ToastData data;
    views::View* view = nullptr;
    base::OneShotTimer timer;
    base::TimeTicks start_time;
  };

  void LayoutToasts();
  void DismissToastById(int64_t id);

  IThemeProvider* theme_ = nullptr;
  views::View* container_ = nullptr;
  std::vector<std::unique_ptr<ActiveToast>> toasts_;
  int64_t next_toast_id_ = 1;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ToastServiceImpl> weak_factory_{this};
};

}  // namespace veor
