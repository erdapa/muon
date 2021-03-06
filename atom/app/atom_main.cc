// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/app/atom_main.h"

#include <stdlib.h>

#include "atom/app/atom_main_delegate.h"
#include "atom/app/uv_task_runner.h"
#include "atom/common/atom_command_line.h"
#include "base/at_exit.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/public/app/content_main.h"

#if defined(OS_WIN)
#include <windows.h>  // windows.h must be included first

#include <shellapi.h>
#include <shellscalingapi.h>
#include <tchar.h>

#include "atom/common/crash_reporter/win/crash_service_main.h"
#include "base/environment.h"
#include "base/win/win_util.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#endif  // defined(OS_WIN)

bool IsEnvSet(const char* name) {
#if defined(OS_WIN)
  size_t required_size;
  getenv_s(&required_size, nullptr, 0, name);
  return required_size != 0;
#else
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
#endif
}

#if defined(OS_MACOSX)
extern "C" {
__attribute__((visibility("default")))
int ChromeMain(int argc, const char* argv[]);
}
#endif  // OS_MACOSX

#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
  int argc = 0;
  wchar_t** argv_setup = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

  if (IsEnvSet("ELECTRON_INTERNAL_CRASH_SERVICE")) {
    return crash_service::Main(cmd);
  }
#else  // OS_WIN
#if defined(OS_MACOSX)
int ChromeMain(int argc, const char* argv[]) {
#else  // OS_MACOSX
int main(int argc, const char* argv[]) {
#endif
  char** argv_setup = uv_setup_args(argc, const_cast<char**>(argv));
#endif  // OS_WIN
  int64_t exe_entry_point_ticks = 0;

  atom::AtomMainDelegate chrome_main_delegate(
      base::TimeTicks::FromInternalValue(exe_entry_point_ticks));
  content::ContentMainParams params(&chrome_main_delegate);

#if defined(OS_WIN)
  install_static::InitializeProductDetailsForPrimaryModule();

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);

  base::win::SetShouldCrashOnProcessDetach(true);
  base::win::SetAbortBehaviorForCrashReporting();

  params.instance = instance;
  params.sandbox_info = &sandbox_info;
  atom::AtomCommandLine::InitW(argc, argv_setup);
#else
  params.argc = argc;
  params.argv = argv;
  atom::AtomCommandLine::Init(argc, argv_setup);
#endif

  base::AtExitManager exit_manager;

  int rv = content::ContentMain(params);

#if defined(OS_WIN)
  base::win::SetShouldCrashOnProcessDetach(false);
#endif

  return rv;
}
