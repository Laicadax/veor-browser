// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "infrastructure/platform/PlatformServiceImpl.h"
#include "base/files/file_path.h"

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/system/sys_info.h"
#include "core/logging/VeorLogger.h"

namespace veor {

PlatformServiceImpl::PlatformServiceImpl() = default;

base::FilePath PlatformServiceImpl::GetUserDataDirectory() const {
  base::FilePath path;
  if (base::PathService::Get(base::DIR_APP_DATA, &path)) {
    return path.AppendASCII("VEOR");
  }
  // Fallback
  return base::FilePath(FILE_PATH_LITERAL(".veor"));
}

base::FilePath PlatformServiceImpl::GetCacheDirectory() const {
  return GetUserDataDirectory().AppendASCII("Cache");
}

base::FilePath PlatformServiceImpl::GetDownloadsDirectory() const {
  base::FilePath path;
  if (base::PathService::Get(base::DIR_USER_DESKTOP, &path)) {
    return path;
  }
  return GetUserDataDirectory().AppendASCII("Downloads");
}

base::FilePath PlatformServiceImpl::GetTempDirectory() const {
  base::FilePath path;
  if (base::PathService::Get(base::DIR_TEMP, &path)) {
    return path.AppendASCII("veor");
  }
  return base::FilePath(FILE_PATH_LITERAL("/tmp/veor"));
}

void PlatformServiceImpl::ShowNotification(const std::string& title,
                                             const std::string& body,
                                             base::OnceCallback<void()> on_click) {
  VEOR_LOGI(LogCategory::kInfrastructure,
            "Notification: [" + title + "] " + body);

#if BUILDFLAG(IS_WIN)
  // Windows: Use ToastNotificationManager or Shell_NotifyIcon
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Windows toast notification would display here");
#elif BUILDFLAG(IS_MAC)
  // macOS: Use NSUserNotificationCenter
  VEOR_LOGD(LogCategory::kInfrastructure,
            "macOS NSUserNotification would display here");
#elif BUILDFLAG(IS_LINUX)
  // Linux: Use libnotify via dbus
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Linux libnotify notification would display here");
#endif

  if (on_click) {
    std::move(on_click).Run();
  }
}

void PlatformServiceImpl::SetBadgeCount(int count) {
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Badge count: " + std::to_string(count));

#if BUILDFLAG(IS_MAC)
  // macOS: [[NSApp dockTile] setBadgeLabel:]
  VEOR_LOGD(LogCategory::kInfrastructure, "macOS dock badge would update here");
#elif BUILDFLAG(IS_WIN)
  // Windows: ITaskbarList3::SetOverlayIcon
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Windows taskbar overlay would update here");
#endif
}

void PlatformServiceImpl::FlashTaskbar(bool flash) {
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Flash taskbar: " + std::string(flash ? "true" : "false"));

#if BUILDFLAG(IS_WIN)
  // Windows: FlashWindowEx
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Windows FlashWindowEx would trigger here");
#elif BUILDFLAG(IS_MAC)
  // macOS: [NSApp requestUserAttention:]
  VEOR_LOGD(LogCategory::kInfrastructure,
            "macOS requestUserAttention would trigger here");
#endif
}

void PlatformServiceImpl::SetAppUserModelId(const std::string& id) {
  VEOR_LOGD(LogCategory::kInfrastructure,
            "AppUserModelId: " + id);

#if BUILDFLAG(IS_WIN)
  // Windows: SetCurrentProcessExplicitAppUserModelID
  VEOR_LOGD(LogCategory::kInfrastructure,
            "Windows AppUserModelID would be set here");
#endif
}

int PlatformServiceImpl::GetLogicalCpuCount() const {
  return base::SysInfo::NumberOfProcessors();
}

size_t PlatformServiceImpl::GetTotalPhysicalMemory() const {
  return base::SysInfo::AmountOfPhysicalMemory();
}

}  // namespace veor
