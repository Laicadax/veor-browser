// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"

#include "security/ISecurityManager.h"
#include "ui/theme/IThemeProvider.h"
#include "ui/views/view.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// PermissionPrompt — Modal dialog for permission requests
// ─────────────────────────────────────────────────────────────────────────────

class PermissionPrompt : public views::View {
 public:
  using DecisionCallback =
      std::function<void(PermissionDecision)>;

  PermissionPrompt(const GURL& origin,
                   PermissionType type,
                   IThemeProvider* theme,
                   DecisionCallback callback);
  ~PermissionPrompt() override;

  void Init();

 private:
  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;

  GURL origin_;
  PermissionType type_;
  raw_ptr<IThemeProvider> theme_;
  DecisionCallback callback_;
};

}  // namespace veor
