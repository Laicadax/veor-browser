// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/VeorBrowserContext.h"

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"

#include "content/VeorResourceContext.h"
#include "core/logging/VeorLogger.h"
#include "safe_browsing/SafeBrowsingService.h"

namespace veor {

namespace {

base::FilePath GetVeorProfilePath() {
  base::FilePath path;
#if defined(OS_WIN)
  base::PathService::Get(base::DIR_LOCAL_APP_DATA, &path);
  path = path.AppendASCII("VEOR");
#elif defined(OS_MAC)
  base::PathService::Get(base::DIR_APP_DATA, &path);
  path = path.AppendASCII("Application Support").AppendASCII("VEOR");
#else
  base::PathService::Get(base::DIR_HOME, &path);
  path = path.AppendASCII(".config").AppendASCII("veor");
#endif
  return path.AppendASCII("Default");
}

// Load Safe Browsing API key from config.json in profile directory.
// Priority: config file > environment variable > fallback.
std::string LoadApiKey(const base::FilePath& profile_path) {
  // 1. Try config.json
  base::FilePath config_path = profile_path.AppendASCII("config.json");
  if (base::PathExists(config_path)) {
    std::string json_str;
    if (base::ReadFileToString(config_path, &json_str)) {
      auto result = base::JSONReader::Read(json_str);
      if (result.has_value() && result->is_dict()) {
        const base::Value::Dict* dict = result->GetIfDict();
        if (dict) {
          const std::string* key = dict->FindString("safe_browsing_api_key");
          if (key && !key->empty()) {
            VEOR_LOGI(LogCategory::kSecurity,
                      "SafeBrowsingService: API key from config.json");
            return *key;
          }
        }
      }
    }
  }

  // 2. Try environment variable
  std::string env_key;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env && env->GetVar("VEOR_SAFE_BROWSING_API_KEY", &env_key) && !env_key.empty()) {
    VEOR_LOGI(LogCategory::kSecurity,
              "SafeBrowsingService: API key from environment");
    return env_key;
  }

  // 3. Built-in fallback — works out of the box, no user action required
  VEOR_LOGI(LogCategory::kSecurity,
            "SafeBrowsingService: using built-in API key");
  return "AIzaSyAFyh4_3KcYWwZ74gbbmvjPLv0H42TgPKc";
}

}  // namespace

VeorBrowserContext::VeorBrowserContext() {
  Initialize();
  resource_context_ = std::make_unique<VeorResourceContext>();
}

VeorBrowserContext::~VeorBrowserContext() = default;

void VeorBrowserContext::Initialize() {
  path_ = GetVeorProfilePath();
  if (!base::PathExists(path_)) {
    base::CreateDirectory(path_);
    VEOR_LOGI(LogCategory::kCore,
              "Created profile directory: " + path_.AsUTF8Unsafe());
  }

  safe_browsing_ = std::make_unique<SafeBrowsingService>(path_);
  if (safe_browsing_->Initialize()) {
    std::string api_key = LoadApiKey(path_);
    safe_browsing_->SetApiKey(api_key);
  } else {
    VEOR_LOGW(LogCategory::kSecurity,
              "SafeBrowsingService initialization failed");
  }
}

base::FilePath VeorBrowserContext::GetPath() {
  return path_;
}

bool VeorBrowserContext::IsOffTheRecord() {
  return false;
}

content::ResourceContext* VeorBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

SafeBrowsingService* VeorBrowserContext::GetSafeBrowsingService() const {
  return safe_browsing_.get();
}

content::DownloadManagerDelegate*
VeorBrowserContext::GetDownloadManagerDelegate() {
  return nullptr;
}

content::BrowserPluginGuestManager* VeorBrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy* VeorBrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

content::PushMessagingService*
VeorBrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::StorageNotificationService*
VeorBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

content::SSLHostStateDelegate*
VeorBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

content::PermissionControllerDelegate*
VeorBrowserContext::GetPermissionControllerDelegate() {
  return nullptr;
}

content::ClientHintsControllerDelegate*
VeorBrowserContext::GetClientHintsControllerDelegate() {
  return nullptr;
}

content::BackgroundFetchDelegate*
VeorBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
VeorBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
VeorBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

content::ReduceAcceptLanguageControllerDelegate*
VeorBrowserContext::GetReduceAcceptLanguageControllerDelegate() {
  return nullptr;
}

content::ResponsivenessObserver*
VeorBrowserContext::GetResponsivenessObserver() {
  return nullptr;
}

content::FileSystemAccessPermissionContext*
VeorBrowserContext::GetFileSystemAccessPermissionContext() {
  return nullptr;
}

content::OriginTrialsControllerDelegate*
VeorBrowserContext::GetOriginTrialsControllerDelegate() {
  return nullptr;
}

}  // namespace veor
