// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>

#include "base/functional/callback.h"

namespace veor {

class IOmnibox {
 public:
  virtual ~IOmnibox() = default;

  virtual void SetText(const std::string& text) = 0;
  virtual std::string GetText() const = 0;

  virtual void SetSecure(bool secure, bool mixed) = 0;
  virtual void SetLoading(bool loading) = 0;

  virtual void Focus() = 0;
  virtual void Blur() = 0;

  virtual void SetOnCommit(base::RepeatingCallback<void(const std::string&)> cb) = 0;
  virtual void SetOnFocusChanged(base::RepeatingCallback<void(bool)> cb) = 0;
};

}  // namespace veor
