// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLAYGROUND_CONTROLLER_H_
#define PLAYGROUND_PLAYGROUND_CONTROLLER_H_

#include <memory>

#include "plugin/plugin_delegate.h"
#include "bindings/runtime.h"

namespace bindings {
class Runtime;
}

namespace plugin {
class PluginController;
}

namespace playground {

// This class represents the main runtime of the Playground plugin. It's owned by the plugin
// controller and, in turn, owns and controls the v8 runtime.
class PlaygroundController : public plugin::PluginDelegate,
                             public bindings::Runtime::Delegate {
 public:
  explicit PlaygroundController(plugin::PluginController* plugin_controller);
  ~PlaygroundController();

  // plugin::PluginDelete implementation.
  void OnCallbacksAvailable(const std::vector<plugin::Callback>& callbacks) override;
  bool OnCallbackIntercepted(const std::string& callback,
                             const plugin::Arguments& arguments) override;
  void OnGamemodeLoaded() override;
  void OnGamemodeUnloaded() override;
  void OnServerFrame() override;

  // bindings::Runtime::Delegate implementation.
  void OnScriptOutput(const std::string& message) override;
  void OnScriptError(const std::string& filename, size_t line_number, const std::string& message) override;
  void OnScriptTestsDone(unsigned int total_tests, unsigned int failed_tests) override;

 private:
  // Weak, owns us. Allows communication with the SA-MP server and Pawn runtime.
  plugin::PluginController* plugin_controller_;

  // The v8 runtime that will be responsible for the JavaScript-based gamemode.
  std::shared_ptr<bindings::Runtime> runtime_;
};

}  // namespace playground

#endif  // PLAYGROUND_PLAYGROUND_CONTROLLER_H_
