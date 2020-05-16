// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_PLUGIN_CONTROLLER_H_
#define PLAYGROUND_PLUGIN_PLUGIN_CONTROLLER_H_

#include <memory>
#include <unordered_map>

#include "plugin/callback_hook.h"

typedef struct tagAMX_NATIVE_INFO AMX_NATIVE_INFO;

namespace base {
class FilePath;
}

namespace plugin {

class Arguments;
struct Callback;
class CallbackManager;
class CallbackParser;
class NativeFunctionManager;
class NativeParser;
class PluginDelegate;

// The plugin controller is responsible for any communication with the SA-MP server and the
// underlying Pawn runtime engine. It's owned by the SA-MP plugin runtime, and will in turn
// own the PlaygroundController which bridges between SA-MP and the v8 runtime.
class PluginController : public CallbackHook::Delegate {
 public:
  explicit PluginController(const base::FilePath& path);
  ~PluginController() override;

  // Gets the table of native AMX functions to be shared with the SA-MP server.
  AMX_NATIVE_INFO* GetNativeTable();

  // Output |message| as a raw string to the console, and record it in the log.
  void Output(const std::string& message) const;

  // Returns whether the player with |player_id| has not recently sent an update to the server.
  bool IsPlayerMinimized(int player_id) const;

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

  // To be called when tests have finished executing in the JavaScript gamemode. The test runner
  // is only interested in running the tests, not the rest of the script.
  void DidRunTests(unsigned int total_tests, unsigned int failed_tests);

  // CallbackHook::Delegate implementation.
  void OnGamemodeChanged(AMX* gamemode) override;
  void OnPlayerUpdate(int player_id) override;
  bool OnCallbackIntercepted(const std::string& callback,
                             const Arguments& arguments,
                             bool deferred) override;

  NativeParser* native_parser() { return native_parser_.get(); }

 private:
  // The hook through which we intercept callbacks issued by the SA-MP server, as well those
  // that are issued through plugins loaded in the SA-MP server.
  std::unique_ptr<CallbackHook> callback_hook_;

  // The parser responsible for identifying the callbacks supported by the plugin.
  std::shared_ptr<CallbackParser> callback_parser_;

  // The callback manager enables JavaScript to call callbacks in the Pawn mode.
  std::unique_ptr<CallbackManager> callback_manager_;

  // The native function manager is responsible for keeping track of available native functions
  // in the gamemode, as well as providing the ability to invoke them when necessary.
  std::unique_ptr<NativeFunctionManager> native_function_manager_;

  // The native function parser that loads the file of functions supported by the plugin.
  std::unique_ptr<NativeParser> native_parser_;

  // The plugin delegate is the higher-level interface for which we translate SA-MP specific
  // concepts to much more generic ones. No traces of the Pawn runtime should be exposed at
  // this layer.
  std::unique_ptr<PluginDelegate> plugin_delegate_;

  // Stores a mapping from a player Id to the time they last sent an update.
  std::unordered_map<int, double> player_update_time_;
};

}  // plugin

#endif  // PLAYGROUND_PLUGIN_PLUGIN_CONTROLLER_H_
