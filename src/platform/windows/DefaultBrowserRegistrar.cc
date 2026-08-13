// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/windows/DefaultBrowserRegistrar.h"

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "core/logging/VeorLogger.h"

namespace veor {

namespace {

constexpr wchar_t kProgId[] = L"VEORBrowser";
constexpr wchar_t kDisplayName[] = L"VEOR Browser";
constexpr wchar_t kAppName[] = L"VEOR";

constexpr wchar_t kProtocols[] = L"http\0https\0";
constexpr wchar_t kExtensions[] = L".htm\0.html\0.shtml\0.xht\0.xhtml\0";

bool IsDefaultForProtocol(const wchar_t* protocol) {
  base::win::RegKey key(HKEY_CURRENT_USER,
                        (std::wstring(L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\") +
                         protocol + L"\\UserChoice")
                            .c_str(),
                        KEY_READ);
  if (!key.Valid())
    return false;

  std::wstring prog_id;
  if (key.ReadValue(L"Progid", &prog_id) != ERROR_SUCCESS)
    return false;

  return prog_id == kProgId;
}

}  // namespace

DefaultBrowserRegistrar::DefaultBrowserRegistrar(
    const base::FilePath& executable_path)
    : executable_path_(executable_path),
      progid_name_(kProgId),
      display_name_(kDisplayName),
      app_name_(kAppName) {}

bool DefaultBrowserRegistrar::IsDefault() const {
  return IsDefaultForProtocol(L"http") && IsDefaultForProtocol(L"https");
}

bool DefaultBrowserRegistrar::Register() const {
  VEOR_LOGI(LogCategory::kPlatform, "Registering VEOR as default browser");

  bool success = true;
  success &= RegisterProgid();
  success &= RegisterCapabilities();
  success &= RegisterStartMenuInternet();
  success &= RegisterFileAssociations();

  if (success) {
    // Set as default for http and https
    success &= SetAsDefaultForProtocol(L"http");
    success &= SetAsDefaultForProtocol(L"https");

    // Set as default for file extensions
    const wchar_t* ext = kExtensions;
    while (*ext) {
      success &= SetAsDefaultForExtension(ext);
      ext += wcslen(ext) + 1;
    }

    NotifyShellOfChange();
    VEOR_LOGI(LogCategory::kPlatform, "VEOR registered as default browser");
  } else {
    VEOR_LOGW(LogCategory::kPlatform,
              "Failed to fully register VEOR as default browser");
  }

  return success;
}

bool DefaultBrowserRegistrar::RegisterProgid() const {
  std::wstring exe_path = base::UTF8ToWide(executable_path_.value());

  // HKEY_CURRENT_USER\Software\Classes\VEORBrowser
  base::win::RegKey progid_key(HKEY_CURRENT_USER,
                               (std::wstring(L"Software\\Classes\\") + kProgId).c_str(),
                               KEY_WRITE);
  if (!progid_key.Valid())
    return false;

  progid_key.WriteValue(L"", display_name_.c_str());
  progid_key.WriteValue(L"FriendlyTypeName", display_name_.c_str());

  // DefaultIcon
  base::win::RegKey icon_key(HKEY_CURRENT_USER,
                             (std::wstring(L"Software\\Classes\\") + kProgId +
                              L"\\DefaultIcon")
                                 .c_str(),
                             KEY_WRITE);
  if (icon_key.Valid()) {
    std::wstring icon_value = exe_path + L",0";
    icon_key.WriteValue(L"", icon_value.c_str());
  }

  // shell\open\command
  base::win::RegKey cmd_key(HKEY_CURRENT_USER,
                            (std::wstring(L"Software\\Classes\\") + kProgId +
                             L"\\shell\\open\\command")
                                .c_str(),
                            KEY_WRITE);
  if (cmd_key.Valid()) {
    std::wstring cmd_value = L"\"" + exe_path + L"\" \"%1\"";
    cmd_key.WriteValue(L"", cmd_value.c_str());
  }

  return true;
}

bool DefaultBrowserRegistrar::RegisterCapabilities() const {
  std::wstring exe_path = base::UTF8ToWide(executable_path_.value());

  // HKEY_CURRENT_USER\Software\Classes\VEORBrowser\Capabilities
  base::win::RegKey caps_key(HKEY_CURRENT_USER,
                             (std::wstring(L"Software\\Classes\\") + kProgId +
                              L"\\Capabilities")
                                 .c_str(),
                             KEY_WRITE);
  if (!caps_key.Valid())
    return false;

  caps_key.WriteValue(L"ApplicationName", app_name_.c_str());
  caps_key.WriteValue(L"ApplicationDescription",
                      L"Precision. Confidence. Architecture. Silence. Restraint.");

  // URLAssociations
  base::win::RegKey url_key(HKEY_CURRENT_USER,
                            (std::wstring(L"Software\\Classes\\") + kProgId +
                             L"\\Capabilities\\URLAssociations")
                                .c_str(),
                            KEY_WRITE);
  if (url_key.Valid()) {
    url_key.WriteValue(L"http", kProgId);
    url_key.WriteValue(L"https", kProgId);
  }

  // FileAssociations
  base::win::RegKey file_key(HKEY_CURRENT_USER,
                             (std::wstring(L"Software\\Classes\\") + kProgId +
                              L"\\Capabilities\\FileAssociations")
                                 .c_str(),
                             KEY_WRITE);
  if (file_key.Valid()) {
    file_key.WriteValue(L".htm", kProgId);
    file_key.WriteValue(L".html", kProgId);
    file_key.WriteValue(L".shtml", kProgId);
    file_key.WriteValue(L".xht", kProgId);
    file_key.WriteValue(L".xhtml", kProgId);
  }

  return true;
}

bool DefaultBrowserRegistrar::RegisterStartMenuInternet() const {
  std::wstring exe_path = base::UTF8ToWide(executable_path_.value());

  // HKEY_CURRENT_USER\Software\Clients\StartMenuInternet\VEOR
  base::win::RegKey client_key(HKEY_CURRENT_USER,
                               (std::wstring(L"Software\\Clients\\StartMenuInternet\\") +
                                app_name_)
                                   .c_str(),
                               KEY_WRITE);
  if (!client_key.Valid())
    return false;

  client_key.WriteValue(L"", display_name_.c_str());

  // DefaultIcon
  base::win::RegKey icon_key(HKEY_CURRENT_USER,
                             (std::wstring(L"Software\\Clients\\StartMenuInternet\\") +
                              app_name_ + L"\\DefaultIcon")
                                 .c_str(),
                             KEY_WRITE);
  if (icon_key.Valid()) {
    std::wstring icon_value = exe_path + L",0";
    icon_key.WriteValue(L"", icon_value.c_str());
  }

  // shell\open\command
  base::win::RegKey cmd_key(HKEY_CURRENT_USER,
                            (std::wstring(L"Software\\Clients\\StartMenuInternet\\") +
                             app_name_ + L"\\shell\\open\\command")
                                .c_str(),
                            KEY_WRITE);
  if (cmd_key.Valid()) {
    std::wstring cmd_value = L"\"" + exe_path + L"\"";
    cmd_key.WriteValue(L"", cmd_value.c_str());
  }

  // Capabilities
  base::win::RegKey caps_key(HKEY_CURRENT_USER,
                             (std::wstring(L"Software\\Clients\\StartMenuInternet\\") +
                              app_name_ + L"\\Capabilities")
                                 .c_str(),
                             KEY_WRITE);
  if (caps_key.Valid()) {
    caps_key.WriteValue(L"ApplicationDescription",
                        L"Precision. Confidence. Architecture. Silence. Restraint.");
  }

  // Register in RegisteredApplications
  base::win::RegKey reg_app_key(HKEY_CURRENT_USER,
                                L"Software\\RegisteredApplications",
                                KEY_WRITE);
  if (reg_app_key.Valid()) {
    reg_app_key.WriteValue(app_name_.c_str(),
                           (std::wstring(L"Software\\Clients\\StartMenuInternet\\") +
                            app_name_ + L"\\Capabilities")
                               .c_str());
  }

  return true;
}

bool DefaultBrowserRegistrar::RegisterFileAssociations() const {
  const wchar_t* ext = kExtensions;
  while (*ext) {
    std::wstring ext_str(ext);
    base::win::RegKey ext_key(HKEY_CURRENT_USER,
                              (std::wstring(L"Software\\Classes\\") + ext_str).c_str(),
                              KEY_WRITE);
    if (ext_key.Valid()) {
      ext_key.WriteValue(L"", kProgId);
      ext_key.WriteValue(L"PerceivedType", L"text");
      ext_key.WriteValue(L"Content Type", L"text/html");
    }
    ext += wcslen(ext) + 1;
  }
  return true;
}

bool DefaultBrowserRegistrar::SetAsDefaultForProtocol(
    const wchar_t* protocol) const {
  // Windows 8+ requires IApplicationAssociationRegistration or
  // IApplicationAssociationRegistrationUI for setting defaults properly.
  // For older Windows, we use registry directly.

  // First, try the modern COM API
  Microsoft::WRL::ComPtr<IApplicationAssociationRegistration> assoc_reg;
  HRESULT hr = CoCreateInstance(
      CLSID_ApplicationAssociationRegistration, nullptr, CLSCTX_INPROC,
      IID_PPV_ARGS(&assoc_reg));

  if (SUCCEEDED(hr) && assoc_reg) {
    hr = assoc_reg->SetAppAsDefault(app_name_.c_str(), protocol, ASSOCIATION_TYPE_URL);
    if (SUCCEEDED(hr)) {
      return true;
    }
  }

  // Fallback: registry manipulation for UserChoice (Windows 7 style)
  // Note: On Windows 10+, UserChoice is protected by hash and requires
  // the COM API. Registry fallback may not work due to hash validation.
  base::win::RegKey choice_key(
      HKEY_CURRENT_USER,
      (std::wstring(L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\") +
       protocol + L"\\UserChoice")
          .c_str(),
      KEY_WRITE);
  if (choice_key.Valid()) {
    choice_key.WriteValue(L"Progid", kProgId);
    return true;
  }

  return false;
}

bool DefaultBrowserRegistrar::SetAsDefaultForExtension(
    const wchar_t* ext) const {
  Microsoft::WRL::ComPtr<IApplicationAssociationRegistration> assoc_reg;
  HRESULT hr = CoCreateInstance(
      CLSID_ApplicationAssociationRegistration, nullptr, CLSCTX_INPROC,
      IID_PPV_ARGS(&assoc_reg));

  if (SUCCEEDED(hr) && assoc_reg) {
    hr = assoc_reg->SetAppAsDefault(app_name_.c_str(), ext, ASSOCIATION_TYPE_FILE);
    if (SUCCEEDED(hr)) {
      return true;
    }
  }

  return false;
}

void DefaultBrowserRegistrar::NotifyShellOfChange() const {
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

void DefaultBrowserRegistrar::OpenDefaultAppsSettings() const {
  // Windows 10+: Open the Default Apps settings page
  ShellExecuteW(nullptr, L"open",
                L"ms-settings:defaultapps",
                nullptr, nullptr, SW_SHOWNORMAL);
}

std::string DefaultBrowserRegistrar::GetStatusMessage() const {
  if (IsDefault()) {
    return "VEOR is your default browser";
  }
  return "VEOR is not your default browser";
}

}  // namespace veor
