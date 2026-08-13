// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <vector>

#include "navigation/INavigationController.h"

namespace veor {

class NavigationControllerImpl : public INavigationController {
 public:
  NavigationControllerImpl();
  ~NavigationControllerImpl() override;

  void PushEntry(const NavigationEntry& entry) override;
  bool CanGoBack() const override;
  bool CanGoForward() const override;
  GURL GoBack() override;
  GURL GoForward() override;
  GURL GetCurrentUrl() const override;

  void SetOnUrlChanged(base::RepeatingCallback<void(const GURL&)> cb) override;

 private:
  std::vector<NavigationEntry> entries_;
  size_t current_index_ = 0;

  base::RepeatingCallback<void(const GURL&)> on_url_changed_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
