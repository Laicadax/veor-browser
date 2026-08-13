// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// INewTabPage
// ─────────────────────────────────────────────────────────────────────────────
// Absolute void. The logo emerges from darkness.
// No shortcuts. No parallax. No SVG geometry.
// Just presence.
// ─────────────────────────────────────────────────────────────────────────────

class INewTabPage {
 public:
  virtual ~INewTabPage() = default;

  virtual void Show() = 0;
  virtual void Hide() = 0;
};

}  // namespace veor
