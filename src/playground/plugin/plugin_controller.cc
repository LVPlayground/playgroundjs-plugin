// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/plugin_controller.h"

#include "base/file_path.h"
#include "playground_controller.h"
#include "plugin/callback_manager.h"
#include "plugin/callback_parser.h"
#include "plugin/native_function_manager.h"
#include "plugin/native_parser.h"
#include "plugin/plugin_delegate.h"
#include "plugin/sdk/plugincommon.h"

extern logprintf_t g_logprintf;
extern DidRunTests_t g_did_run_tests;

namespace plugin {

namespace {

// File in which the list of to-be-forwarded callbacks are listed.
const char kCallbackFile[] = "data/server/callbacks.txt";

// File in which the list of provided native functions are listed.
const char kNativesFile[] = "data/server/natives.txt";

// Maximum number of bytes to send in a single logprintf() call.
const size_t kLogLimit = 2048;

}  // namespace

PluginController::PluginController(const base::FilePath& path) {
  // Initialize the callback parser for |kCallbackFile| in the current |path|.
  callback_parser_.reset(CallbackParser::FromFile(path.Append(kCallbackFile)));
  if (!callback_parser_) {
    LOG(FATAL) << "Unable to initialize the callback parser. Does " << kCallbackFile << " exist?";
    return;
  }

  // Initialize the callback hook with ourselves as the delegate, and the parsed callbacks.
  callback_hook_.reset(new CallbackHook(this, callback_parser_));
  if (!callback_hook_->Install()) {
    LOG(FATAL) << "Unable to install the callback hook in the SA-MP server.";
    return;
  }

  // Initialize the native function manager, enabling us to use native functions.
  native_function_manager_.reset(new NativeFunctionManager);
  if (!native_function_manager_->Install()) {
    LOG(FATAL) << "Unable to install the native hook in the SA-MP server.";
    return;
  }

  // Initialize the callback manager, which can call public functions in all available AMX files.
  callback_manager_.reset(new CallbackManager);

  // Initialize the plugin delegate, which is the PlaygroundController.
  plugin_delegate_.reset(new playground::PlaygroundController(this));
  plugin_delegate_->OnCallbacksAvailable(callback_parser_->callbacks());

  // Initialize the native functions that are to be provided by the plugin.
  native_parser_ = NativeParser::FromFile(path.Append(kNativesFile));
  if (!native_parser_) {
    LOG(FATAL) << "Unable to initialize the native parser. Does " << kNativesFile << " exist?";
    return;
  }

  // If the test runner is driving this invocation, announce availability of the gamemode.
  if (!pAMXFunctions)
    plugin_delegate_->OnGamemodeLoaded();
}

PluginController::~PluginController() {}

// Gets the table of native AMX functions to be shared with the SA-MP server.
AMX_NATIVE_INFO* PluginController::GetNativeTable() {
  return native_parser_->GetNativeTable();
}

void PluginController::Output(const std::string& message) const {
  // The SA-MP server exposes the logprintf(), but does not document that the maximum length of a
  // string is 2048 bytes. As such, split up the message in 2048 chunks if it's larger, and issue
  // multiple calls to the logprintf() function to avoid this limitation.
  if (message.size() < kLogLimit) {
    g_logprintf("%s", message.c_str());
    return;
  }

  size_t offset = 0;
  for (size_t offset = 0; offset < message.size(); offset += kLogLimit)
    g_logprintf("%s", message.substr(offset, kLogLimit).c_str());
}

bool PluginController::FunctionExists(const std::string& function_name) const {
  return native_function_manager_->FunctionExists(function_name);
}

int PluginController::CallFunction(const std::string& function_name, const char* format, void** arguments) {
  if (function_name.size() > 2 && function_name[0] == 'O' && function_name[1] == 'n')
    return callback_manager_->CallPublic(function_name, format, arguments);

  return native_function_manager_->CallFunction(function_name, format, arguments);
}

void PluginController::OnServerFrame() {
  plugin_delegate_->OnServerFrame();
}

void PluginController::DidRunTests(unsigned int total_tests, unsigned int failed_tests) {
  if (!pAMXFunctions)
    g_did_run_tests(total_tests, failed_tests);
}

void PluginController::OnGamemodeChanged(AMX* gamemode) {
  callback_manager_->OnGamemodeChanged(gamemode);
  if (gamemode)
    plugin_delegate_->OnGamemodeLoaded();
}

bool PluginController::OnCallbackIntercepted(const std::string& callback,
                                             const Arguments& arguments) {
  return plugin_delegate_->OnCallbackIntercepted(callback, arguments);
}

}  // namespace plugin
