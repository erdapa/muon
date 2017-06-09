// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_content_client.h"

#include <string>
#include <vector>

#include "atom/common/atom_version.h"
#include "atom/common/options_switches.h"
#include "atom/common/pepper_flash_util.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_version.h"
#include "chrome/common/crash_keys.h"
#include "chrome/common/extensions/extension_process_policy.h"
#include "chrome/common/secure_origin_whitelist.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/pepper_plugin_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/features/features.h"
#include "gpu/config/gpu_crash_keys.h"
#include "gpu/config/gpu_info.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/url_constants.h"
#include "widevine_cdm_version.h"  // NOLINT

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "extensions/common/features/feature_util.h"
#endif

namespace atom {

namespace {
void ConvertStringWithSeparatorToVector(std::vector<std::string>* vec,
                                        const char* separator,
                                        const char* cmd_switch) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto string_with_separator = command_line->GetSwitchValueASCII(cmd_switch);
  if (!string_with_separator.empty())
    *vec = base::SplitString(string_with_separator, separator,
                             base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);
}

}  // namespace


AtomContentClient::AtomContentClient() {
}

AtomContentClient::~AtomContentClient() {
}

void AtomContentClient::SetGpuInfo(const gpu::GPUInfo& gpu_info) {
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUVendorID,
      base::StringPrintf("0x%04x", gpu_info.gpu.vendor_id));
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUDeviceID,
      base::StringPrintf("0x%04x", gpu_info.gpu.device_id));
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUDriverVersion,
      gpu_info.driver_version);
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUPixelShaderVersion,
      gpu_info.pixel_shader_version);
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUVertexShaderVersion,
      gpu_info.vertex_shader_version);
#if defined(OS_MACOSX)
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUGLVersion,
      gpu_info.gl_version);
#elif defined(OS_POSIX)
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPUVendor,
      gpu_info.gl_vendor);
  base::debug::SetCrashKeyValue(gpu::crash_keys::kGPURenderer,
      gpu_info.gl_renderer);
#endif
}

std::string AtomContentClient::GetProduct() const {
  return "Chrome/" CHROME_VERSION_STRING;
}

std::string AtomContentClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      "Chrome/" CHROME_VERSION_STRING);
}

bool AtomContentClient::IsSupplementarySiteIsolationModeEnabled() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return true;
#else
  return false;
#endif
}

void AtomContentClient::AddAdditionalSchemes(Schemes* schemes) {
  schemes->standard_schemes.push_back(extensions::kExtensionScheme);
  schemes->savable_schemes.push_back(extensions::kExtensionScheme);
  schemes->secure_schemes.push_back(extensions::kExtensionScheme);
  schemes->secure_origins = GetSecureOriginWhitelist();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (extensions::feature_util::ExtensionServiceWorkersEnabled())
    schemes->service_worker_schemes.push_back(extensions::kExtensionScheme);

  // As far as Blink is concerned, they should be allowed to receive CORS
  // requests. At the Extensions layer, requests will actually be blocked unless
  // overridden by the web_accessible_resources manifest key.
  // TODO(kalman): See what happens with a service worker.
  schemes->cors_enabled_schemes.push_back(extensions::kExtensionScheme);
#endif
}

bool AtomContentClient::AllowScriptExtensionForServiceWorker(
    const GURL& script_url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return script_url.SchemeIs(extensions::kExtensionScheme);
#else
  return false;
#endif
}

content::OriginTrialPolicy* AtomContentClient::GetOriginTrialPolicy() {
  if (!origin_trial_policy_) {
    origin_trial_policy_ = base::WrapUnique(new ChromeOriginTrialPolicy());
  }
  return origin_trial_policy_.get();
}

void AtomContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
  AddPepperFlashFromCommandLine(plugins);
}

}  // namespace atom
