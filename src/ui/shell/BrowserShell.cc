// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/shell/BrowserShell.h"

#include "base/command_line.h"
#include "base/strings/escape.h"
#include "content/public/browser/browser_context.h"
#include "core/base/UrlSecurity.h"
#include "ui/base/accelerators/accelerator.h"
#include "command/providers/SystemCommandProvider.h"
#include "devtools/DevToolsManager.h"
#include "platform/IDefaultBrowserRegistrar.h"
#include "bookmarks/BookmarkStoreImpl.h"
#include "bookmarks/BookmarkStoreStub.h"
#include "history/HistoryStoreImpl.h"
#include "history/HistoryStoreStub.h"
#include "infrastructure/storage/StorageEngineImpl.h"
#include "autofill/AutofillManager.h"
#include "autofill/AutofillStoreImpl.h"
#include "downloads/DownloadController.h"
#include "infrastructure/crypto/CryptoVaultImpl.h"
#include "update/UpdateChecker.h"
#include "ui/blocked/BlockedPageView.h"
#include "ui/bookmarks/BookmarksView.h"
#include "ui/content/WebContentsView.h"
#include "ui/downloads/DownloadsView.h"
#include "ui/find/FindBarView.h"
#include "ui/settings/SettingsView.h"
#include "ui/gfx/canvas.h"
#include "ui/history/HistoryView.h"
#include "ui/ntp/NewTabPageView.h"
#include "ui/omnibox/OmniboxView.h"
#include "ui/palette/CommandPaletteView.h"
#include "ui/shell/TitleBar.h"
#include "tabs/TabManager.h"
#include "ui/components/TabStripView.h"
#include "ui/theme/IThemeProvider.h"
#include "ui/toast/ToastServiceImpl.h"
#include "ui/views/background.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "workspace/IWorkspace.h"
#include "workspace/IWorkspaceManager.h"

#include <algorithm>

namespace veor {

BEGIN_METADATA(BrowserShell)
END_METADATA

BrowserShell::BrowserShell(std::unique_ptr<IThemeProvider> theme,
                           IWorkspaceManager* workspace_manager,
                           content::BrowserContext* browser_context,
                           SafeBrowsingService* safe_browsing)
    : theme_(std::move(theme)),
      workspace_manager_(workspace_manager),
      browser_context_(browser_context),
      safe_browsing_(safe_browsing) {
  DCHECK(theme_);
  DCHECK(workspace_manager_);
  DCHECK(browser_context_);
  InitTabSystem();
  InitLayout();
  InitOverlays();
}

BrowserShell::~BrowserShell() = default;

std::unique_ptr<views::NonClientFrameView>
BrowserShell::CreateNonClientFrameView(views::Widget* widget) {
  return nullptr;
}

void BrowserShell::InitTabSystem() {
  auto workspaces = workspace_manager_->GetAllWorkspaces();
  if (!workspaces.empty()) {
    auto* ws = workspaces[0];
    if (ws) {
      tab_manager_ = ws->GetTabManager();
      // Create initial tab
      CreateNewTab(GURL());
    }
  }
}

void BrowserShell::InitLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  // TitleBar
  title_bar_ = AddChildView(std::make_unique<TitleBar>(theme_.get()));
  title_bar_->SetOnBackPressed(
      base::BindRepeating(&BrowserShell::OnBackPressed,
                          base::Unretained(this)));
  title_bar_->SetOnForwardPressed(
      base::BindRepeating(&BrowserShell::OnForwardPressed,
                          base::Unretained(this)));
  title_bar_->SetOnCommandPaletteRequested(
      base::BindRepeating(&BrowserShell::OnCommandPaletteRequested,
                          base::Unretained(this)));
  title_bar_->SetOnCloseRequested(base::BindRepeating(
      [](BrowserShell* self) {
        if (self->GetWidget())
          self->GetWidget()->CloseWithReason(
              views::Widget::ClosedReason::kCloseButtonClicked);
      },
      base::Unretained(this)));

  // Wire omnibox (now inside TitleBar)
  auto* omnibox = title_bar_->GetOmnibox();
  omnibox->SetOnCommit(
      base::BindRepeating(&BrowserShell::OnOmniboxCommit,
                          base::Unretained(this)));
  omnibox->SetOnFocusChanged(
      base::BindRepeating(&BrowserShell::OnOmniboxFocusChanged,
                          base::Unretained(this)));

  // TabStrip
  tab_strip_ = AddChildView(std::make_unique<TabStripView>(theme_.get()));
  tab_strip_->SetOnTabSelected(
      base::BindRepeating(&BrowserShell::SwitchToTab,
                          base::Unretained(this)));
  tab_strip_->SetOnTabClosed(
      base::BindRepeating(&BrowserShell::CloseTab,
                          base::Unretained(this)));
  tab_strip_->SetOnTabContextMenu(
      base::BindRepeating(&BrowserShell::OnTabContextMenu,
                          base::Unretained(this)));

  // ContentArea — flexes to fill remaining space
  content_area_ = AddChildView(std::make_unique<views::View>());
  content_area_->SetBackground(
      views::CreateSolidBackground(theme_->GetColor(ColorRole::kVoid)));
  layout->SetFlexForView(content_area_, 1);

  // Set workspace name
  auto workspaces = workspace_manager_->GetAllWorkspaces();
  if (!workspaces.empty()) {
    auto* ws = workspaces[0];
    if (ws)
      title_bar_->SetWorkspaceName(ws->GetName());
  }
}

void BrowserShell::InitOverlays() {
  overlay_layer_ = AddChildView(std::make_unique<views::View>());
  overlay_layer_->SetVisible(true);
  overlay_layer_->SetBackground(
      views::CreateSolidBackground(SK_ColorTRANSPARENT));
  overlay_layer_->SetPaintToLayer();
  overlay_layer_->layer()->SetFillsBoundsOpaquely(false);

  // Command Palette — hidden by default
  command_palette_ = overlay_layer_->AddChildView(
      std::make_unique<CommandPaletteView>(theme_.get()));
  command_palette_->SetOnItemSelected(
      base::BindRepeating(&BrowserShell::OnPaletteItemSelected,
                          base::Unretained(this)));
  command_palette_->SetOnQueryChanged(
      base::BindRepeating(&BrowserShell::OnPaletteQueryChanged,
                          base::Unretained(this)));

  // Register system commands (default browser, reader mode, etc.)
  auto registrar = CreateDefaultBrowserRegistrar(
      base::CommandLine::ForCurrentProcess()->GetProgram());
  if (registrar) {
    command_palette_->RegisterProvider(
        std::make_unique<SystemCommandProvider>(
            std::move(registrar),
            base::BindRepeating(&BrowserShell::ToggleReaderMode,
                                base::Unretained(this))));
  }

  // New Tab Page — shown by default when no active content
  new_tab_page_ = overlay_layer_->AddChildView(
      std::make_unique<NewTabPageView>(theme_.get()));
  new_tab_page_->Show();

  // Toast service
  toast_service_ = std::make_unique<ToastServiceImpl>(theme_.get());
  toast_service_->SetContainer(overlay_layer_);

  // DevTools
  devtools_manager_ = std::make_unique<DevToolsManager>(browser_context_);

  // Download controller
  download_controller_ = std::make_unique<DownloadController>();

  // Update checker
  update_checker_ = std::make_unique<UpdateChecker>();
  update_checker_->CheckForUpdate(
      base::BindOnce([](BrowserShell* self, bool available,
                        const std::string& version) {
        if (available) {
          self->ShowToast("Update available", "VEOR " + version, 10000);
        }
      }, base::Unretained(this)));

  // Crypto vault for autofill encryption
  crypto_vault_ = std::make_unique<CryptoVaultImpl>();

  // Autofill store (shares storage engine)
  autofill_store_ = std::make_unique<AutofillStoreImpl>(
      storage_engine_.get(), crypto_vault_.get());

  // Storage — initialize SQLite-backed stores with fallback to stubs
  storage_engine_ = std::make_unique<StorageEngineImpl>();
  base::FilePath storage_path;
  if (browser_context_) {
    storage_path = browser_context_->GetPath().Append(
        FILE_PATH_LITERAL("veor.db"));
  } else {
    storage_path = base::FilePath(FILE_PATH_LITERAL("/tmp/veor_default.db"));
  }
  auto open_result = storage_engine_->Open(storage_path);
  if (open_result.IsOk()) {
    history_store_ = std::make_unique<HistoryStoreImpl>(storage_engine_.get());
    bookmark_store_ = std::make_unique<BookmarkStoreImpl>(storage_engine_.get());
    VEOR_LOGI(LogCategory::kInfrastructure,
              "Storage initialized at " + storage_path.value());
  } else {
    VEOR_LOGW(LogCategory::kInfrastructure,
              "Failed to open storage, using stubs: " +
                  open_result.UnwrapErr().ToString());
    history_store_ = std::make_unique<HistoryStoreStub>();
    bookmark_store_ = std::make_unique<BookmarkStoreStub>();
  }

  // History view — hidden by default
  history_view_ = overlay_layer_->AddChildView(
      std::make_unique<HistoryView>(theme_.get(), history_store_.get()));
  history_view_->SetOnEntrySelected(
      base::BindRepeating(&BrowserShell::OnOmniboxCommit,
                          base::Unretained(this)));
  history_view_->Hide();

  // Bookmarks view — hidden by default
  bookmarks_view_ = overlay_layer_->AddChildView(
      std::make_unique<BookmarksView>(theme_.get(), bookmark_store_.get()));
  bookmarks_view_->SetOnBookmarkSelected(
      base::BindRepeating(&BrowserShell::OnOmniboxCommit,
                          base::Unretained(this)));
  bookmarks_view_->Hide();

  // Blocked page — hidden by default
  blocked_page_ = overlay_layer_->AddChildView(
      std::make_unique<BlockedPageView>(theme_.get()));
  blocked_page_->SetVisible(false);

  // Find bar — hidden by default, anchored at bottom of content area
  find_bar_ = overlay_layer_->AddChildView(
      std::make_unique<FindBarView>(theme_.get()));
  find_bar_->SetOnFindRequested(
      base::BindRepeating(&BrowserShell::OnFindRequested,
                          base::Unretained(this)));
  find_bar_->SetOnCloseRequested(
      base::BindRepeating(&BrowserShell::HideFindBar,
                          base::Unretained(this)));
  find_bar_->SetVisible(false);

  // Downloads view — hidden by default
  downloads_view_ = overlay_layer_->AddChildView(
      std::make_unique<DownloadsView>(theme_.get(), download_controller_.get()));
  downloads_view_->SetVisible(false);

  // Settings view — hidden by default
  // TODO: Get settings provider from active workspace
  settings_view_ = overlay_layer_->AddChildView(
      std::make_unique<SettingsView>(theme_.get(), nullptr));
  settings_view_->SetVisible(false);
}

void BrowserShell::OnPaint(gfx::Canvas* canvas) {
  canvas->FillRect(GetLocalBounds(),
                   theme_->GetColor(ColorRole::kVoid));
}

void BrowserShell::Layout() {
  views::View::Layout();

  if (overlay_layer_) {
    overlay_layer_->SetBounds(GetLocalBounds());

    if (command_palette_)
      command_palette_->SetBounds(overlay_layer_->GetLocalBounds());

    if (new_tab_page_) {
      gfx::Rect ntp_bounds = content_area_->GetMirroredBounds();
      ntp_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      new_tab_page_->SetBounds(ntp_bounds);
    }
    if (history_view_) {
      gfx::Rect panel_bounds = content_area_->GetMirroredBounds();
      panel_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      history_view_->SetBounds(panel_bounds);
    }
    if (bookmarks_view_) {
      gfx::Rect panel_bounds = content_area_->GetMirroredBounds();
      panel_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      bookmarks_view_->SetBounds(panel_bounds);
    }
    if (blocked_page_) {
      gfx::Rect content_bounds = content_area_->GetMirroredBounds();
      content_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      blocked_page_->SetBounds(content_bounds);
    }
    if (find_bar_ && find_bar_->IsVisible()) {
      gfx::Size bar_size = find_bar_->GetPreferredSize();
      gfx::Rect content_bounds = content_area_->GetMirroredBounds();
      content_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      find_bar_->SetBounds(
          content_bounds.x(),
          content_bounds.bottom() - bar_size.height(),
          content_bounds.width(),
          bar_size.height());
    }
    if (downloads_view_ && downloads_view_->IsVisible()) {
      gfx::Rect panel_bounds = content_area_->GetMirroredBounds();
      panel_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      int panel_width = std::min(400, panel_bounds.width() / 2);
      panel_bounds.set_x(panel_bounds.right() - panel_width);
      panel_bounds.set_width(panel_width);
      downloads_view_->SetBounds(panel_bounds);
    }
    if (settings_view_ && settings_view_->IsVisible()) {
      gfx::Rect panel_bounds = content_area_->GetMirroredBounds();
      panel_bounds.Offset(-GetMirroredPosition().x(), -GetMirroredPosition().y());
      // Centered modal-like panel
      int panel_w = std::min(640, panel_bounds.width() - 80);
      int panel_h = std::min(480, panel_bounds.height() - 80);
      panel_bounds.set_x(panel_bounds.x() + (panel_bounds.width() - panel_w) / 2);
      panel_bounds.set_y(panel_bounds.y() + (panel_bounds.height() - panel_h) / 2);
      panel_bounds.set_width(panel_w);
      panel_bounds.set_height(panel_h);
      settings_view_->SetBounds(panel_bounds);
    }
  }

  // Layout content views within content_area_
  gfx::Rect content_bounds = content_area_->GetLocalBounds();
  for (auto& view : web_contents_views_) {
    view->SetBounds(content_bounds);
  }

  // Position tab context menu if visible
  if (tab_context_menu_ && tab_context_menu_->GetVisible()) {
    gfx::Size menu_size = tab_context_menu_->GetPreferredSize();
    gfx::Rect menu_bounds = tab_context_menu_->bounds();
    if (menu_bounds.right() > width())
      menu_bounds.set_x(width() - menu_size.width() - 4);
    if (menu_bounds.bottom() > height())
      menu_bounds.set_y(height() - menu_size.height() - 4);
    tab_context_menu_->SetBounds(menu_bounds);
  }
}

void BrowserShell::AddedToWidget() {
  views::WidgetDelegateView::AddedToWidget();

  auto* fm = GetWidget()->GetFocusManager();
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_T, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_L, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_I,
                      ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_H, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_B,
                      ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_W, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_R, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_T,
                      ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_TAB, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_TAB,
                      ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_F5, ui::EF_NONE),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_F12, ui::EF_NONE),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_F, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_OEM_PLUS, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_OEM_MINUS, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_0, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_F11, ui::EF_NONE),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_J, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
  fm->RegisterAccelerator(
      ui::Accelerator(ui::VKEY_OEM_COMMA, ui::EF_CONTROL_DOWN),
      ui::AcceleratorManager::kNormalPriority, this);
}

bool BrowserShell::AcceleratorPressed(const ui::Accelerator& accelerator) {
  if (accelerator.key_code() == ui::VKEY_T &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    CreateNewTab(GURL());
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_L &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    title_bar_->GetOmnibox()->Focus();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_I &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN) &&
      (accelerator.modifiers() & ui::EF_SHIFT_DOWN)) {
    if (active_tab_index_ < web_contents_views_.size() &&
        devtools_manager_) {
      devtools_manager_->ToggleFor(
          web_contents_views_[active_tab_index_]->GetWebContents());
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_H &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    if (history_view_) {
      bool visible = history_view_->GetVisible();
      history_view_->SetVisible(!visible);
      if (!visible) history_view_->Reload();
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_B &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN) &&
      (accelerator.modifiers() & ui::EF_SHIFT_DOWN)) {
    if (bookmarks_view_) {
      bool visible = bookmarks_view_->GetVisible();
      bookmarks_view_->SetVisible(!visible);
      if (!visible) bookmarks_view_->Reload();
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_W &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    if (has_tabs_)
      CloseTab(active_tab_index_);
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_R &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    if (active_tab_index_ < web_contents_views_.size())
      web_contents_views_[active_tab_index_]->Reload();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_T &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN) &&
      (accelerator.modifiers() & ui::EF_SHIFT_DOWN)) {
    ReopenClosedTab();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_TAB &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    if (accelerator.modifiers() & ui::EF_SHIFT_DOWN) {
      if (web_contents_views_.size() > 1) {
        size_t prev = active_tab_index_ == 0
                          ? web_contents_views_.size() - 1
                          : active_tab_index_ - 1;
        SwitchToTab(prev);
      }
    } else {
      if (web_contents_views_.size() > 1) {
        size_t next = (active_tab_index_ + 1) % web_contents_views_.size();
        SwitchToTab(next);
      }
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_ESCAPE &&
      accelerator.modifiers() == ui::EF_NONE) {
    CloseAllOverlays();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_F5 &&
      accelerator.modifiers() == ui::EF_NONE) {
    if (active_tab_index_ < web_contents_views_.size())
      web_contents_views_[active_tab_index_]->Reload();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_F12 &&
      accelerator.modifiers() == ui::EF_NONE) {
    if (active_tab_index_ < web_contents_views_.size() && devtools_manager_) {
      devtools_manager_->ToggleFor(
          web_contents_views_[active_tab_index_]->GetWebContents());
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_F &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    ShowFindBar();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_OEM_PLUS &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    ZoomIn();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_OEM_MINUS &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    ZoomOut();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_0 &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    ResetZoom();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_F11 &&
      accelerator.modifiers() == ui::EF_NONE) {
    ToggleFullscreen();
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_J &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    if (downloads_view_) {
      bool visible = downloads_view_->IsVisible();
      if (visible) {
        downloads_view_->Hide();
      } else {
        downloads_view_->Show();
      }
    }
    return true;
  }
  if (accelerator.key_code() == ui::VKEY_OEM_COMMA &&
      (accelerator.modifiers() & ui::EF_CONTROL_DOWN)) {
    if (settings_view_) {
      bool visible = settings_view_->IsVisible();
      if (visible) {
        settings_view_->Hide();
      } else {
        settings_view_->Show();
      }
    }
    return true;
  }
  return views::WidgetDelegateView::AcceleratorPressed(accelerator);
}

// ── Navigation ───────────────────────────────────────────────────────────────

void BrowserShell::OnBackPressed() {
  if (active_tab_index_ < web_contents_views_.size() &&
      web_contents_views_[active_tab_index_]->CanGoBack()) {
    web_contents_views_[active_tab_index_]->GoBack();
  }
  UpdateTabVisuals();
}

void BrowserShell::OnForwardPressed() {
  if (active_tab_index_ < web_contents_views_.size() &&
      web_contents_views_[active_tab_index_]->CanGoForward()) {
    web_contents_views_[active_tab_index_]->GoForward();
  }
  UpdateTabVisuals();
}

void BrowserShell::OnOmniboxCommit(const std::string& text) {
  GURL url(text);
  // Text typed or pasted into the omnibox must never execute script in, or
  // borrow the origin of, the page currently loaded in the tab.
  if (IsDangerousNavigationScheme(url)) {
    VEOR_LOGW(LogCategory::kSecurity,
              "Omnibox rejected " + url.scheme() + ": URL");
    url = GURL();
  }
  if (!url.is_valid()) {
    bool looks_like_url = text.find('.') != std::string::npos &&
                          text.find(' ') == std::string::npos &&
                          text.find("://") == std::string::npos;
    if (looks_like_url) {
      url = GURL("https://" + text);
    } else {
      // Search engine fallback
      std::string engine = "google";
      auto workspaces = workspace_manager_->GetAllWorkspaces();
      if (!workspaces.empty() && workspaces[0]->GetSettings()) {
        engine = workspaces[0]->GetSettings()->GetString(
            "search.default_engine", "google");
      }

      std::string query = base::EscapeQueryParamValue(text, false);
      if (engine == "duckduckgo") {
        url = GURL("https://duckduckgo.com/?q=" + query);
      } else if (engine == "bing") {
        url = GURL("https://www.bing.com/search?q=" + query);
      } else {
        url = GURL("https://www.google.com/search?q=" + query);
      }
    }
  }

  if (!url.is_valid())
    return;

  // Safe Browsing check before navigation
  if (safe_browsing_) {
    auto result = safe_browsing_->CheckUrlSync(url);
    if (result.threat_type != ThreatType::kSafe) {
      VEOR_LOGW(LogCategory::kSecurity,
                "BrowserShell blocked: " + url.spec());
      ShowBlockedPage(url, static_cast<int>(result.threat_type));
      title_bar_->GetOmnibox()->Blur();
      UpdateTabVisuals();
      return;
    }
  }

  if (!has_tabs_) {
    CreateNewTab(url);
  } else if (active_tab_index_ < web_contents_views_.size()) {
    web_contents_views_[active_tab_index_]->LoadUrl(url);
  }

  title_bar_->GetOmnibox()->Blur();
  UpdateTabVisuals();
}

void BrowserShell::OnOmniboxFocusChanged(bool focused) {
  // Subtle: could dim content area when omnibox is focused
}

// ── Tab Context Menu ─────────────────────────────────────────────────────────

void BrowserShell::OnTabContextMenu(size_t index, gfx::Point point) {
  if (index >= web_contents_views_.size())
    return;

  // Dismiss existing menu
  if (tab_context_menu_) {
    tab_context_menu_->SetVisible(false);
    overlay_layer_->RemoveChildView(tab_context_menu_);
    tab_context_menu_ = nullptr;
  }

  // Create inline context menu
  auto menu = std::make_unique<views::View>();
  menu->SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurface)));
  menu->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(2)));

  auto AddItem = [&](const std::u16string& label,
                     base::RepeatingClosure action) {
    auto* btn = menu->AddChildView(
        std::make_unique<views::LabelButton>(std::move(action), label));
    btn->SetTextColor(views::Button::STATE_NORMAL,
                      theme_->GetColor(ColorRole::kTextPrimary));
    btn->SetTextColor(views::Button::STATE_HOVERED,
                      theme_->GetColor(ColorRole::kTextPrimary));
    btn->SetBackground(views::CreateSolidBackground(SK_ColorTRANSPARENT));
    btn->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    btn->SetPreferredSize(gfx::Size(140, 28));
    return btn;
  };

  bool is_pinned = false;  // TODO: track pinned state in TabVisual
  if (is_pinned) {
    AddItem(u"Unpin", base::BindRepeating(
        [](BrowserShell* self, size_t idx) {
          self->tab_context_menu_->SetVisible(false);
        }, base::Unretained(this), index));
  } else {
    AddItem(u"Pin", base::BindRepeating(
        [](BrowserShell* self, size_t idx) {
          self->tab_context_menu_->SetVisible(false);
        }, base::Unretained(this), index));
  }

  AddItem(u"Duplicate", base::BindRepeating(
      [](BrowserShell* self, size_t idx) {
        if (idx < self->web_contents_views_.size()) {
          GURL url = self->web_contents_views_[idx]->GetCurrentUrl();
          self->CreateNewTab(url);
        }
        self->tab_context_menu_->SetVisible(false);
      }, base::Unretained(this), index));

  AddItem(u"Reload", base::BindRepeating(
      [](BrowserShell* self, size_t idx) {
        if (idx < self->web_contents_views_.size())
          self->web_contents_views_[idx]->Reload();
        self->tab_context_menu_->SetVisible(false);
      }, base::Unretained(this), index));

  AddItem(u"Close", base::BindRepeating(
      [](BrowserShell* self, size_t idx) {
        self->CloseTab(idx);
        self->tab_context_menu_->SetVisible(false);
      }, base::Unretained(this), index));

  AddItem(u"Close Others", base::BindRepeating(
      [](BrowserShell* self, size_t keep_idx) {
        for (int i = static_cast<int>(self->web_contents_views_.size()) - 1;
             i >= 0; --i) {
          if (static_cast<size_t>(i) != keep_idx)
            self->CloseTab(i);
        }
        self->tab_context_menu_->SetVisible(false);
      }, base::Unretained(this), index));

  // Position menu near click point
  gfx::Point menu_pos = point;
  views::View::ConvertPointToTarget(tab_strip_, overlay_layer_, &menu_pos);
  menu->SetBounds(menu_pos.x(), menu_pos.y() + tab_strip_->height(),
                  144, 28 * 5 + 4);

  tab_context_menu_ = overlay_layer_->AddChildView(std::move(menu));
  tab_context_menu_->SetVisible(true);
}

void BrowserShell::OnAutofillSavePrompt(const std::string& origin,
                                        const std::string& username,
                                        const std::string& password) {
  ShowToast("Save password?",
            origin,
            6000,
            base::BindRepeating(
                [](BrowserShell* self, const std::string& o,
                   const std::string& u, const std::string& p) {
                  if (self->autofill_store_) {
                    PasswordEntry entry;
                    entry.origin = o;
                    entry.username = u;
                    entry.password_encrypted = p;
                    self->autofill_store_->SavePassword(entry);
                    self->ShowToast("Password saved", o, 2000);
                  }
                },
                base::Unretained(this), origin, username, password),
            "Save");
}

// ── Safe Browsing ──────────────────────────────────────────────────────────────

void BrowserShell::ShowBlockedPage(const GURL& url, int threat_type) {
  if (blocked_page_) {
    blocked_page_->SetBlockedUrl(url.spec(), threat_type);
    blocked_page_->SetVisible(true);
    for (auto& view : web_contents_views_) {
      view->SetVisible(false);
    }
    if (new_tab_page_)
      new_tab_page_->SetVisible(false);
    if (history_view_)
      history_view_->SetVisible(false);
    if (bookmarks_view_)
      bookmarks_view_->SetVisible(false);
  }
  // Toast notification
  ShowToast("Blocked by Safe Browsing", url.host(), 5000);
}

void BrowserShell::ToggleReaderMode() {
  if (active_tab_index_ >= web_contents_views_.size())
    return;
  auto* view = web_contents_views_[active_tab_index_].get();
  if (!view)
    return;
  auto* wc = view->GetWebContents();
  if (!wc)
    return;
  wc->GetPrimaryMainFrame()->ExecuteJavaScript(
      u"window.__veor_reader.toggle();",
      base::NullCallback());
}

// ── Command Palette ──────────────────────────────────────────────────────────

void BrowserShell::OnCommandPaletteRequested() {
  if (command_palette_) {
    command_palette_->Show();
    command_palette_->SetQuery("");
    command_palette_->SetResults({});
  }
}

void BrowserShell::OnPaletteItemSelected(int index) {
  if (!command_palette_)
    return;

  auto results = BuildPaletteResults(command_palette_->GetQuery());
  if (index < 0 || static_cast<size_t>(index) >= results.size())
    return;

  const auto& item = results[index];
  if (item.type == "command") {
    if (item.title == "> New Tab") {
      CreateNewTab(GURL());
    } else if (item.title == "> Close Tab") {
      if (has_tabs_)
        CloseTab(active_tab_index_);
    } else if (item.title == "> Reload") {
      if (active_tab_index_ < web_contents_views_.size())
        web_contents_views_[active_tab_index_]->Reload();
    } else if (item.title == "> Command Palette") {
      // Already open
    } else if (item.title == "> History") {
      if (history_view_) {
        history_view_->SetVisible(true);
        history_view_->Reload();
      }
    } else if (item.title == "> Bookmarks") {
      if (bookmarks_view_) {
        bookmarks_view_->SetVisible(true);
        bookmarks_view_->Reload();
      }
    }
  } else if (item.type == "tab") {
    // Switch to tab by URL match
    for (size_t i = 0; i < web_contents_views_.size(); ++i) {
      if (web_contents_views_[i]->GetCurrentUrl().spec() == item.subtitle) {
        SwitchToTab(i);
        break;
      }
    }
  }

  if (command_palette_)
    command_palette_->Hide();
}

void BrowserShell::OnPaletteQueryChanged(const std::string& query) {
  if (command_palette_)
    command_palette_->SetResults(BuildPaletteResults(query));
}

std::vector<PaletteItem> BrowserShell::BuildPaletteResults(
    const std::string& query) {
  std::vector<PaletteItem> results;
  std::string q = query;
  std::transform(q.begin(), q.end(), q.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Commands — always available, prefixed with ">"
  const std::vector<std::pair<std::string, std::string>> commands = {
      {"> New Tab", "Open a new tab"},
      {"> Close Tab", "Close the active tab"},
      {"> Reload", "Reload the current page"},
      {"> Command Palette", "Open this palette"},
      {"> History", "Open browsing history"},
      {"> Bookmarks", "Open bookmarks"},
  };

  for (const auto& cmd : commands) {
    std::string title_lower = cmd.first;
    std::transform(title_lower.begin(), title_lower.end(),
                   title_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (q.empty() || title_lower.find(q) != std::string::npos) {
      results.push_back({cmd.first, cmd.second, "command"});
    }
  }

  // Open tabs
  for (size_t i = 0; i < web_contents_views_.size(); ++i) {
    GURL url = web_contents_views_[i]->GetCurrentUrl();
    std::string title = url.host().empty() ? "New Tab" : url.host();
    std::string title_lower = title;
    std::transform(title_lower.begin(), title_lower.end(),
                   title_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::string url_lower = url.spec();
    std::transform(url_lower.begin(), url_lower.end(),
                   url_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (q.empty() || title_lower.find(q) != std::string::npos ||
        url_lower.find(q) != std::string::npos) {
      results.push_back({title, url.spec(), "tab"});
    }
  }

  return results;
}

// ── Tab System ───────────────────────────────────────────────────────────────

void BrowserShell::CreateNewTab(const GURL& url) {
  // Create WebContentsView
  auto wc_view = std::make_unique<WebContentsView>(browser_context_, theme_.get());
  wc_view->SetOnTitleChanged(
      base::BindRepeating(&BrowserShell::OnContentTitleChanged,
                          base::Unretained(this)));
  wc_view->SetOnUrlChanged(
      base::BindRepeating(&BrowserShell::OnContentUrlChanged,
                          base::Unretained(this)));
  wc_view->SetOnLoadingStateChanged(
      base::BindRepeating(&BrowserShell::OnContentLoadingChanged,
                          base::Unretained(this)));

  // Add to content area (hidden initially except active)
  content_area_->AddChildView(wc_view.get());

  if (url.is_valid()) {
    wc_view->LoadUrl(url);
  }

  web_contents_views_.push_back(std::move(wc_view));

  active_tab_index_ = web_contents_views_.size() - 1;
  has_tabs_ = true;

  // Hide NTP, show content
  if (new_tab_page_)
    new_tab_page_->Hide();

  // Create autofill manager for this tab
  auto* wc = wc_view->GetWebContents();
  if (wc && autofill_store_) {
    auto manager = std::make_unique<AutofillManager>(
        wc, autofill_store_.get(), theme_.get());
    manager->SetOnSavePromptRequested(
        base::BindRepeating(&BrowserShell::OnAutofillSavePrompt,
                            base::Unretained(this)));
    autofill_managers_.push_back(std::move(manager));
  }

  UpdateTabVisuals();
  Layout();
}

void BrowserShell::SwitchToTab(size_t index) {
  if (index >= web_contents_views_.size())
    return;

  active_tab_index_ = index;

  // Show active content, hide others
  for (size_t i = 0; i < web_contents_views_.size(); ++i) {
    web_contents_views_[i]->SetVisible(i == index);
  }

  // Update omnibox with current URL
  GURL url = web_contents_views_[index]->GetCurrentUrl();
  if (url.is_valid())
    title_bar_->GetOmnibox()->SetText(url.spec());

  UpdateTabVisuals();
}

void BrowserShell::CloseTab(size_t index) {
  if (index >= web_contents_views_.size())
    return;

  // Save URL for reopen
  GURL closed_url = web_contents_views_[index]->GetCurrentUrl();
  std::string closed_title = web_contents_views_[index]->GetCurrentUrl().host();
  if (closed_url.is_valid()) {
    closed_tabs_.push_back({closed_url, closed_title});
    if (closed_tabs_.size() > 10)
      closed_tabs_.erase(closed_tabs_.begin());
  }

  // Show toast with undo action
  ShowToast("Tab closed", closed_title.empty() ? "New Tab" : closed_title,
            4000,
            base::BindRepeating(&BrowserShell::ReopenClosedTab,
                                base::Unretained(this)),
            "Undo");

  // Remove content view from hierarchy (parent does not own it)
  content_area_->RemoveChildView(web_contents_views_[index].get());
  web_contents_views_.erase(web_contents_views_.begin() + index);

  // Remove corresponding autofill manager
  if (index < autofill_managers_.size()) {
    autofill_managers_.erase(autofill_managers_.begin() + index);
  }

  if (web_contents_views_.empty()) {
    has_tabs_ = false;
    active_tab_index_ = 0;
    if (new_tab_page_)
      new_tab_page_->Show();
  } else {
    if (active_tab_index_ >= web_contents_views_.size())
      active_tab_index_ = web_contents_views_.size() - 1;
    // Show active tab
    for (size_t i = 0; i < web_contents_views_.size(); ++i) {
      web_contents_views_[i]->SetVisible(i == active_tab_index_);
    }
    // Update omnibox
    GURL url = web_contents_views_[active_tab_index_]->GetCurrentUrl();
    if (url.is_valid())
      title_bar_->GetOmnibox()->SetText(url.spec());
  }

  UpdateTabVisuals();
}

void BrowserShell::UpdateTabVisuals() {
  std::vector<TabStripView::TabVisual> visuals;
  for (size_t i = 0; i < web_contents_views_.size(); ++i) {
    TabStripView::TabVisual v;
    GURL url = web_contents_views_[i]->GetCurrentUrl();
    v.title = url.host();
    if (v.title.empty())
      v.title = "New Tab";
    v.url_host = url.host();
    v.active = (i == active_tab_index_);
    v.pinned = false;
    v.audio_playing = false;
    // Domain color from hash
    uint32_t hash = std::hash<std::string>{}(v.url_host);
    v.domain_color = SkColorSetRGB(
        static_cast<uint8_t>((hash >> 16) & 0x7F) + 0x40,
        static_cast<uint8_t>((hash >> 8) & 0x7F) + 0x40,
        static_cast<uint8_t>(hash & 0x7F) + 0x40);
    visuals.push_back(v);
  }

  if (tab_strip_)
    tab_strip_->SetTabs(visuals);

  // Update title bar nav state
  if (active_tab_index_ < web_contents_views_.size()) {
    title_bar_->SetCanGoBack(
        web_contents_views_[active_tab_index_]->CanGoBack());
    title_bar_->SetCanGoForward(
        web_contents_views_[active_tab_index_]->CanGoForward());
  }
}

void BrowserShell::OnContentTitleChanged(const std::string& title) {
  UpdateTabVisuals();
}

void BrowserShell::OnContentUrlChanged(const GURL& url) {
  if (active_tab_index_ < web_contents_views_.size()) {
    title_bar_->GetOmnibox()->SetText(url.spec());
  }
  // Record history visit
  if (history_store_ && url.is_valid() && !url.host().empty()) {
    std::string title = url.host();
    if (active_tab_index_ < web_contents_views_.size()) {
      // TODO: Get actual page title from WebContentsView when API available
    }
    history_store_->AddVisit(url, title, base::Time::Now());
  }
  UpdateTabVisuals();
}

void BrowserShell::OnContentLoadingChanged(bool loading) {
  title_bar_->GetOmnibox()->SetLoading(loading);
}

void BrowserShell::CloseAllOverlays() {
  if (tab_context_menu_) {
    tab_context_menu_->SetVisible(false);
  }
  if (command_palette_ && command_palette_->IsVisible())
    command_palette_->Hide();
  if (history_view_ && history_view_->GetVisible())
    history_view_->SetVisible(false);
  if (bookmarks_view_ && bookmarks_view_->GetVisible())
    bookmarks_view_->SetVisible(false);
  if (downloads_view_ && downloads_view_->IsVisible())
    downloads_view_->Hide();
  if (settings_view_ && settings_view_->IsVisible())
    settings_view_->Hide();
  if (blocked_page_ && blocked_page_->GetVisible()) {
    blocked_page_->SetVisible(false);
    for (auto& view : web_contents_views_) {
      view->SetVisible(true);
    }
    if (new_tab_page_ && !has_tabs_)
      new_tab_page_->SetVisible(true);
  }
}

void BrowserShell::ReopenClosedTab() {
  if (closed_tabs_.empty())
    return;
  ClosedTab tab = std::move(closed_tabs_.back());
  closed_tabs_.pop_back();
  CreateNewTab(tab.url);
}

void BrowserShell::ShowToast(const std::string& title,
                             const std::string& detail,
                             int duration_ms,
                             base::RepeatingClosure action,
                             const std::string& action_label) {
  if (!toast_service_)
    return;
  ToastData toast;
  toast.title = title;
  toast.detail = detail;
  toast.duration_ms = duration_ms;
  toast.action = std::move(action);
  toast.action_label = action_label;
  toast_service_->Show(toast);
}

// ── Find in Page ───────────────────────────────────────────────────────────────

void BrowserShell::ShowFindBar() {
  if (!find_bar_)
    return;
  find_bar_->Show();
  Layout();
  find_bar_->FocusSearchBox();
}

void BrowserShell::HideFindBar() {
  if (!find_bar_)
    return;
  find_bar_->Hide();
  if (active_tab_index_ < web_contents_views_.size())
    web_contents_views_[active_tab_index_]->StopFinding();
  Layout();
}

void BrowserShell::OnFindRequested(const std::string& text,
                                   bool forward,
                                   bool match_case) {
  if (active_tab_index_ < web_contents_views_.size()) {
    web_contents_views_[active_tab_index_]->Find(text, forward, match_case);
    web_contents_views_[active_tab_index_]->SetOnFindResult(
        base::BindRepeating(&BrowserShell::OnFindResult,
                            base::Unretained(this)));
  }
}

void BrowserShell::OnFindResult(int active_match, int matches) {
  if (find_bar_)
    find_bar_->SetMatchCount(active_match, matches);
}

// ── Zoom ───────────────────────────────────────────────────────────────────────

void BrowserShell::ZoomIn() {
  if (active_tab_index_ < web_contents_views_.size())
    web_contents_views_[active_tab_index_]->ZoomIn();
}

void BrowserShell::ZoomOut() {
  if (active_tab_index_ < web_contents_views_.size())
    web_contents_views_[active_tab_index_]->ZoomOut();
}

void BrowserShell::ResetZoom() {
  if (active_tab_index_ < web_contents_views_.size())
    web_contents_views_[active_tab_index_]->ResetZoom();
}

// ── Fullscreen ─────────────────────────────────────────────────────────────────

void BrowserShell::ToggleFullscreen() {
  auto* widget = GetWidget();
  if (!widget)
    return;
  if (widget->IsFullscreen()) {
    widget->SetFullscreen(false);
  } else {
    widget->SetFullscreen(true);
  }
}

}  // namespace veor
