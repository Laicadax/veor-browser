// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/IDefaultBrowserRegistrar.h"
#include "base/files/file_path.h"

#if BUILDFLAG(IS_WIN)
#include "platform/windows/DefaultBrowserRegistrar.h"
#endif

namespace veor {

std::unique_ptr<IDefaultBrowserRegistrar> CreateDefaultBrowserRegistrar(
    const base::FilePath& executable_path) {
#if BUILDFLAG(IS_WIN)
  return std::make_unique<DefaultBrowserRegistrar>(executable_path);
#else
  return nullptr;
#endif
}

}  // namespace veor
