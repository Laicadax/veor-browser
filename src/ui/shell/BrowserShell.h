// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <memory>
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"
#include <unordered_map>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/widget/widget_delegate.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace veor {

class BlockedPageView;
class BookmarksView;
class DevToolsManager;
class HistoryView;
class IAutofillStore;
class IBookmarkStore;
class ICryptoVault;
class IDownloadController;
class IHistoryStore;
class IStorageEngine;
class IThemeProvider;
class IWorkspaceManager;
class SafeBrowsingService;
class TitleBar;
class TabStripView;
class CommandPaletteView;
class DownloadsView;
class FindBarView;
class SettingsView;
class NewTabPageView;
class ToastServiceImpl;
class UpdateChecker;
class WebContentsView;
class ITabManager;
class AutofillManager;

// ─────────────────────────────────────────────────────────────────────────────
// BrowserShell — Composite Container (V10.5)
// ─────────────────────────────────────────────────────────────────────────────
// Hierarchy:
//   TitleBar (36px) — contains OmniboxView integrated
//   TabStripView
//   ContentArea (flex) — holds WebContentsView
//   OverlayLayer (z-index above all)
//     CommandPaletteView
//     NewTabPageView
//     Toast container
// ─────────────────────────────────────────────────────────────────────────────

class BrowserShell : public views::WidgetDelegateView {
  METADATA_HEADER(BrowserShell, views::WidgetDelegateView)

 public:
  BrowserShell(std::unique_ptr<IThemeProvider> theme,
               IWorkspaceManager* workspace_manager,
               content::BrowserContext* browser_context,
               SafeBrowsingService* safe_browsing);
  ~BrowserShell() override;

  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;

  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;
  void AddedToWidget() override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

 private:
  void InitLayout();
  void InitOverlays();
  void InitTabSystem();

  // Navigation
  void OnBackPressed();
  void OnForwardPressed();
  void OnOmniboxCommit(const std::string& text);
  void OnOmniboxFocusChanged(bool focused);

  // Command Palette
  void OnCommandPaletteRequested();
  void OnPaletteItemSelected(int index);
  void OnPaletteQueryChanged(const std::string& query);
  std::vector<PaletteItem> BuildPaletteResults(const std::string& query);

  // Tab system
  void CreateNewTab(const GURL& url);
  void SwitchToTab(size_t index);
  void CloseTab(size_t index);
  void ReopenClosedTab();
  void UpdateTabVisuals();
  void OnContentTitleChanged(const std::string& title);
  void OnContentUrlChanged(const GURL& url);
  void OnContentLoadingChanged(bool loading);

  // Overlays
  void CloseAllOverlays();

  // Safe Browsing
  void ShowBlockedPage(const GURL& url, int threat_type);

  // Reader Mode
  void ToggleReaderMode();

  // Toast notifications
  void ShowToast(const std::string& title, const std::string& detail,
                 int duration_ms = 4000,
                 base::RepeatingClosure action = base::RepeatingClosure(),
                 const std::string& action_label = "");

  // Tab context menu
  void OnTabContextMenu(size_t index, gfx::Point point);

  // Autofill
  void OnAutofillSavePrompt(const std::string& origin,
                            const std::string& username,
                            const std::string& password);

  // Find in page
  void ShowFindBar();
  void HideFindBar();
  void OnFindRequested(const std::string& text, bool forward, bool match_case);
  void OnFindResult(int active_match, int matches);

  // Zoom
  void ZoomIn();
  void ZoomOut();
  void ResetZoom();

  // Fullscreen
  void ToggleFullscreen();

  std::unique_ptr<IThemeProvider> theme_;
  raw_ptr<IWorkspaceManager> workspace_manager_= nullptr;
  raw_ptr<ITabManager> tab_manager_= nullptr;
  raw_ptr<content::BrowserContext> browser_context_= nullptr;
  raw_ptr<SafeBrowsingService> safe_browsing_= nullptr;

  // Main layout
  raw_ptr<TitleBar> title_bar_= nullptr;
  raw_ptr<TabStripView> tab_strip_= nullptr;
  raw_ptr<views::View> content_area_= nullptr;

  // Content views per tab (index → view)
  std::vector<std::unique_ptr<WebContentsView>> web_contents_views_;
  size_t active_tab_index_ = 0;
  bool has_tabs_ = false;

  // Closed tabs stack for Ctrl+Shift+T
  struct ClosedTab {
    GURL url;
    std::string title;
  };
  std::vector<ClosedTab> closed_tabs_;

  // Overlay layer
  raw_ptr<views::View> overlay_layer_= nullptr;
  raw_ptr<CommandPaletteView> command_palette_= nullptr;
  raw_ptr<DownloadsView> downloads_view_= nullptr;
  raw_ptr<FindBarView> find_bar_= nullptr;
  raw_ptr<SettingsView> settings_view_= nullptr;
  raw_ptr<NewTabPageView> new_tab_page_= nullptr;
  raw_ptr<HistoryView> history_view_= nullptr;
  raw_ptr<BookmarksView> bookmarks_view_= nullptr;
  raw_ptr<BlockedPageView> blocked_page_= nullptr;
  raw_ptr<views::View> tab_context_menu_= nullptr;

  // Services
  std::unique_ptr<ToastServiceImpl> toast_service_;
  std::unique_ptr<DevToolsManager> devtools_manager_;
  std::unique_ptr<IDownloadController> download_controller_;
  std::unique_ptr<UpdateChecker> update_checker_;
  std::unique_ptr<IHistoryStore> history_store_;
  std::unique_ptr<IBookmarkStore> bookmark_store_;
  std::unique_ptr<IStorageEngine> storage_engine_;
  std::unique_ptr<ICryptoVault> crypto_vault_;
  std::unique_ptr<IAutofillStore> autofill_store_;
  std::vector<std::unique_ptr<AutofillManager>> autofill_managers_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
