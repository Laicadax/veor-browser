// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"
#include <vector>

#include "base/functional/callback.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace veor {

class IThemeProvider;
class IHistoryStore;
struct HistoryEntry;

// ─────────────────────────────────────────────────────────────────────────────
// HistoryView
// ─────────────────────────────────────────────────────────────────────────────
// Displays browsing history as a scrollable list.
// Opens via Command Palette "> History" or Ctrl+H.
// ─────────────────────────────────────────────────────────────────────────────

class HistoryView : public views::View {
  METADATA_HEADER(HistoryView, views::View)

 public:
  explicit HistoryView(IThemeProvider* theme,
                       IHistoryStore* history_store);
  ~HistoryView() override;

  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;

  // Refresh from database
  void Reload();

  // Callback when user selects an entry
  void SetOnEntrySelected(base::RepeatingCallback<void(const GURL&)> cb);

  // Mouse
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  void DrawHeader(gfx::Canvas* canvas);
  void DrawEntries(gfx::Canvas* canvas);
  void LoadEntries();

  raw_ptr<IThemeProvider> theme_= nullptr;
  raw_ptr<HistoryDatabase> history_db_= nullptr;

  std::vector<HistoryEntry> entries_;
  base::RepeatingCallback<void(const GURL&)> on_entry_selected_;

  int scroll_offset_ = 0;
  int entry_height_ = 40;
  int header_height_ = 48;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
