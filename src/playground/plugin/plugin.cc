// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include <memory>

#include "base/file_path.h"
#include "base/logging.h"
#include "plugin/plugin_controller.h"
#include "plugin/sdk/amx.h"
#include "plugin/sdk/plugincommon.h"

// Logging handler exported by the SA-MP server.
logprintf_t g_logprintf = nullptr;

// For announcing that the JavaScript tests have finished executing.
DidRunTests_t g_did_run_tests = nullptr;

namespace {

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

}  // namespace

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
  g_logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
  g_did_run_tests = (DidRunTests_t) ppData[PLUGIN_DATA_DID_RUN_TESTS];

  base::FilePath::Initialize();

  // Override the destination for LOG() messages throughout this plugin's source.
  logging::LogMessage::SetLogHandler(std::unique_ptr<SAMPLogHandler>(new SAMPLogHandler));

  // Initialize the PluginController. This will (synchronously) initialize many of the systems
  // required to load the v8 runtime when the gamemode gets loaded.
  g_plugin_controller.reset(new plugin::PluginController(base::FilePath::CurrentDirectory()));

  return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  g_plugin_controller.reset();
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() {
  DCHECK(g_plugin_controller);
  g_plugin_controller->OnServerFrame();
}
