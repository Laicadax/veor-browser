// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/files/file_path.h"
#include "platform/IDefaultBrowserRegistrar.h"

#include "platform/IDefaultBrowserRegistrar.h"

#include "platform/IDefaultBrowserRegistrar.h"

namespace veor {

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// DefaultBrowserRegistrar
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Registers VEOR as the default browser on Windows through registry
// manipulation and COM APIs.
//
// Protocols registered: http, https
// File associations: .htm, .html, .shtml, .xht, .xhtml
//
// Thread safety: [UI Thread] for all methods.

class DefaultBrowserRegistrar : public IDefaultBrowserRegistrar {
 public:
  explicit DefaultBrowserRegistrar(const base::FilePath& executable_path);
  ~DefaultBrowserRegistrar() = default;

  // Returns true if VEOR is currently the default browser.
  bool IsDefault() const;

  // Registers VEOR as the default browser for all supported protocols
  // and file types. Returns true on success.
  bool Register() const;

  // Opens the Windows Default Apps settings page.
  void OpenDefaultAppsSettings() const;

  // Returns a human-readable status message.
  std::string GetStatusMessage() const;

 private:
  bool RegisterProgid() const;
  bool RegisterCapabilities() const;
  bool RegisterStartMenuInternet() const;
  bool RegisterFileAssociations() const;
  bool SetAsDefaultForProtocol(const wchar_t* protocol) const;
  bool SetAsDefaultForExtension(const wchar_t* ext) const;
  void NotifyShellOfChange() const;

  base::FilePath executable_path_;
  std::wstring progid_name_;
  std::wstring display_name_;
  std::wstring app_name_;
};

}  // namespace veor
