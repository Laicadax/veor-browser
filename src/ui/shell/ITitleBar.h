// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>
#include <vector>

#include "base/functional/callback.h"

namespace veor {

class ITitleBar {
 public:
  virtual ~ITitleBar() = default;

  virtual void SetCanGoBack(bool can) = 0;
  virtual void SetCanGoForward(bool can) = 0;
  virtual void SetWorkspaceName(const std::string& name) = 0;
  virtual void SetWorkspaceList(const std::vector<std::string>& names,
                                size_t active_index) = 0;

  virtual void SetOnBackPressed(base::RepeatingClosure cb) = 0;
  virtual void SetOnForwardPressed(base::RepeatingClosure cb) = 0;
  virtual void SetOnWorkspaceSelected(base::RepeatingCallback<void(size_t)> cb) = 0;
  virtual void SetOnCommandPaletteRequested(base::RepeatingClosure cb) = 0;
  virtual void SetOnCloseRequested(base::RepeatingClosure cb) = 0;
};

}  // namespace veor
