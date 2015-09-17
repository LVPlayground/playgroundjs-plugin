// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/plugin_controller.h"

#include "base/file_path.h"
#include "playground_controller.h"
#include "plugin/callback_parser.h"
#include "plugin/native_function_manager.h"
#include "plugin/plugin_delegate.h"
#include "plugin/sdk/plugincommon.h"

extern logprintf_t g_logprintf;

namespace plugin {

namespace {

// File in which the list of to-be-forwarded callbacks are listed.
const char kCallbackFile[] = "callbacks.txt";

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

  // Initialize the plugin delegate, which is the PlaygroundController.
  plugin_delegate_.reset(new playground::PlaygroundController(this));
  plugin_delegate_->OnCallbacksAvailable(callback_parser_->callbacks());
}

PluginController::~PluginController() {}

void PluginController::Output(const char* message) const {
  g_logprintf("%s", message);
}

bool PluginController::FunctionExists(const std::string& function_name) const {
  return native_function_manager_->FunctionExists(function_name);
}

int PluginController::CallFunction(const std::string& function_name, const char* format, void** arguments) {
  return native_function_manager_->CallFunction(function_name, format, arguments);
}

void PluginController::OnServerFrame() {
  plugin_delegate_->OnServerFrame();
}

void PluginController::OnGamemodeChanged(AMX* gamemode) {
  if (gamemode)
    plugin_delegate_->OnGamemodeLoaded();
  else
    plugin_delegate_->OnGamemodeUnloaded();
}

bool PluginController::OnCallbackIntercepted(const std::string& callback,
                                             const Arguments& arguments) {
  return plugin_delegate_->OnCallbackIntercepted(callback, arguments);
}

}  // namespace plugin
