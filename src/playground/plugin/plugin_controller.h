// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_PLUGIN_CONTROLLER_H_
#define PLAYGROUND_PLUGIN_PLUGIN_CONTROLLER_H_

#include <memory>

#include "plugin/callback_hook.h"

namespace base {
class FilePath;
}

namespace plugin {

class Arguments;
struct Callback;
class CallbackParser;
class NativeFunctionManager;
class PluginDelegate;

// The plugin controller is responsible for any communication with the SA-MP server and the
// underlying Pawn runtime engine. It's owned by the SA-MP plugin runtime, and will in turn
// own the PlaygroundController which bridges between SA-MP and the v8 runtime.
class PluginController : public CallbackHook::Delegate {
 public:
  explicit PluginController(const base::FilePath& path);
  ~PluginController() override;

  // Output |message| as a raw string to the console, and record it in the log.
  void Output(const char* message) const;

  // Returns whether a function named |function_name| exists in the Pawn runtime.
  bool FunctionExists(const std::string& function_name) const;
  
  // Calls the Pawn function named |function_name|, having |arguments| structured like |format|. The
  // return value of the function will be returned, whereas any arguments passed by reference will
  // have their values updated accordingly. This method does not rely on having a live gamemode.
  int CallFunction(const std::string& function_name,
                   const char* format = nullptr,
                   void** arguments = nullptr);

  // Called when the SA-MP server starts delivering a frame on the main thread.
  void OnServerFrame();

  // CallbackHook::Delegate implementation.
  void OnGamemodeChanged(AMX* gamemode) override;
  bool OnCallbackIntercepted(const std::string& callback,
                             const Arguments& arguments) override;

 private:
  // The hook through which we intercept callbacks issued by the SA-MP server, as well those
  // that are issued through plugins loaded in the SA-MP server.
  std::unique_ptr<CallbackHook> callback_hook_;

  // The parser responsible for identifying the callbacks supported by the plugin.
  std::shared_ptr<CallbackParser> callback_parser_;

  // The native function manager is responsible for keeping track of available native functions
  // in the gamemode, as well as providing the ability to invoke them when necessary.
  std::unique_ptr<NativeFunctionManager> native_function_manager_;

  // The plugin delegate is the higher-level interface for which we translate SA-MP specific
  // concepts to much more generic ones. No traces of the Pawn runtime should be exposed at
  // this layer.
  std::unique_ptr<PluginDelegate> plugin_delegate_;
};

}  // plugin

#endif  // PLAYGROUND_PLUGIN_PLUGIN_CONTROLLER_H_
