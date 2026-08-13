// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/files/file_path.h"
#include "content/public/browser/browser_context.h"

namespace veor {

class SafeBrowsingService;
class VeorResourceContext;

// ─────────────────────────────────────────────────────────────────────────────
// VeorBrowserContext
// ─────────────────────────────────────────────────────────────────────────────
// VEOR browser profile. Owns:
//   - Path to profile data (~/.config/veor/Default/)
//   - ResourceContext (for URL requests)
//   - RequestContextGetter (Stage 3: Network Service)
//
// Not off-the-record. All data persisted to disk.
// ─────────────────────────────────────────────────────────────────────────────

class VeorBrowserContext : public content::BrowserContext {
 public:
  VeorBrowserContext();
  ~VeorBrowserContext() override;

  // content::BrowserContext
  base::FilePath GetPath() override;
  bool IsOffTheRecord() override;
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::StorageNotificationService* GetStorageNotificationService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate() override;
  content::ClientHintsControllerDelegate* GetClientHintsControllerDelegate() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate() override;
  content::ReduceAcceptLanguageControllerDelegate*
  GetReduceAcceptLanguageControllerDelegate() override;
  content::ResponsivenessObserver* GetResponsivenessObserver() override;
  content::FileSystemAccessPermissionContext*
  GetFileSystemAccessPermissionContext() override;
  content::OriginTrialsControllerDelegate* GetOriginTrialsControllerDelegate() override;

  // Safe Browsing
  SafeBrowsingService* GetSafeBrowsingService() const;

  // Initialize profile directory
  void Initialize();

 private:
  base::FilePath path_;
  std::unique_ptr<SafeBrowsingService> safe_browsing_;
  std::unique_ptr<VeorResourceContext> resource_context_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace veor
