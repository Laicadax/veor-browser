// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.

#include <memory>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "content/public/app/content_main.h"
#include "content/public/app/content_main_delegate.h"

#include "content/VeorMainDelegate.h"

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

  auto delegate = std::make_unique<veor::VeorMainDelegate>();
  content::ContentMainParams params(delegate.get());
  params.argc = argc;
  params.argv = const_cast<const char**>(argv);

  return content::ContentMain(std::move(params));
}
