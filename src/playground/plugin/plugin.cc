// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include <memory>

#include "base/encoding.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/time.h"
#include "bindings/provided_natives.h"
#include "plugin/native_parameters.h"
#include "plugin/native_parser.h"
#include "plugin/plugin_controller.h"
#include "plugin/sdk/amx.h"
#include "plugin/sdk/plugincommon.h"

#include <boost/asio.hpp>

#if defined(LINUX)
#include <sys/resource.h>
#endif

// Logging handler exported by the SA-MP server.
logprintf_t g_logprintf = nullptr;

// For announcing that the JavaScript tests have finished executing.
DidRunTests_t g_did_run_tests = nullptr;

using bindings::ProvidedNatives;

namespace {

#define CHECK_PARAMS(n) { if (params[0] != n * sizeof(cell)) { g_logprintf("SCRIPT: Bad parameter count (%d != %d): ", params[0], n); return 0; } }

// Global instance of the PluginController class, which will be kept alive for the duration of this
// plugin being loaded by the SA-MP server.
std::unique_ptr<plugin::PluginController> g_plugin_controller;

// Logging handler which overrides the destination of LOG() statements within the source of
// this plugin. It will make sure all of them end up in the SA-MP server's log files.
class SAMPLogHandler : public logging::LogMessage::LogHandler {
 public:
  SAMPLogHandler() {}
  ~SAMPLogHandler() override {}

  void Write(const char* severity, const char* file, unsigned int line, const char* message) override {
    g_logprintf("[%s][%s:%d] %s", severity, file, line, message);
  }
};

bool GetStringFromPawnArg(AMX* amx, cell param, std::string* result) {
  result->clear();

  cell* amx_addr = 0;
  int amx_len = 0;

  amx_GetAddr(amx, param, &amx_addr);
  amx_StrLen(amx_addr, &amx_len);

  if (!amx_len)
    return false;  // empty string

  char* buffer = (char*) alloca(amx_len + 1);
  if (buffer && amx_GetString(buffer, amx_addr, 0, amx_len + 1) == AMX_ERR_NONE) {
    result->assign(buffer, amx_len);  // skip the null terminator
    return true;
  }

  return false;
}

// native IsPlayerMinimized(playerid);
static cell AMX_NATIVE_CALL n_IsPlayerMinimized(AMX* amx, cell* params) {
  CHECK_PARAMS(1);

  if (g_plugin_controller) {
    return g_plugin_controller->IsPlayerMinimized(
      params[1], base::monotonicallyIncreasingTime()) ? 1 : 0;
  }

  return 0;
}

}  // namespace

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
  g_logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
  g_did_run_tests = (DidRunTests_t) ppData[PLUGIN_DATA_DID_RUN_TESTS];

  initializeEncoding();

  base::FilePath::Initialize();

  // Make sure that the core limit is set to unlimited when running the plugin on Linux. This
  // will enable the operating system from creating core dump in case of crashes.
#if defined(LINUX)
  struct rlimit core_limits;
  core_limits.rlim_cur = RLIM_INFINITY;
  core_limits.rlim_max = RLIM_INFINITY;

  setrlimit(RLIMIT_CORE, &core_limits);
#endif

  // Override the destination for LOG() messages throughout this plugin's source.
  logging::LogMessage::SetLogHandler(std::unique_ptr<SAMPLogHandler>(new SAMPLogHandler));

  // Initialize the PluginController. This will (synchronously) initialize many of the systems
  // required to load the v8 runtime when the gamemode gets loaded.
  g_plugin_controller.reset(new plugin::PluginController(base::FilePath::CurrentDirectory()));

  // Register the static native functions provided by the plugin's C++ code.
  g_plugin_controller->native_parser()->SetStaticNative(/* index= */ 0, "IsPlayerMinimized", n_IsPlayerMinimized);

  return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  g_plugin_controller.reset();
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  if (g_plugin_controller)
    return amx_Register(amx, g_plugin_controller->GetNativeTable(), -1);

  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() {
  if (g_plugin_controller)
    g_plugin_controller->OnServerFrame();
}
