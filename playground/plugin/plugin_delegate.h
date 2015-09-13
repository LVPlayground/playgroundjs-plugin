// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_PLUGIN_DELEGATE_H_
#define PLAYGROUND_PLUGIN_PLUGIN_DELEGATE_H_

#include <vector>

namespace plugin {

class Arguments;
struct Callback;

// The plugin delegate creates a bridge between the plugin layer and the v8 runtime layer.
class PluginDelegate {
 public:
  virtual ~PluginDelegate() {}

  // Called when a list of callbacks that will be intercepted is available. Will always be called
  // before the gamemode has been loaded into the server.
  virtual void OnCallbacksAvailable(const std::vector<Callback>& callbacks) = 0;

  // Called when a callback has been intercepted. The |callback| contains information about the
  // event that has been invoked, the |arguments| contain the actual context. Returning true
  // from this method will block the callback from being invoked in the Pawn runtime.
  virtual bool OnCallbackIntercepted(const Callback& callback, const Arguments& arguments) = 0;

  // Called when the gamemode has been loaded. This is the appropriate time to initialize the v8
  // runtime, as callbacks will start to be invoked shortly after this.
  virtual void OnGamemodeLoaded() = 0;

  // Called when the gamemode is being unloaded. This is the appropriate time to shut down the
  // v8 runtime to avoid losing unsaved data.
  virtual void OnGamemodeUnloaded() = 0;

  // Called when the server begins a new frame on the main thread. Beyond intercepted callbacks,
  // this is the only time where it's OK to invoke native functions.
  virtual void OnServerFrame() = 0;
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_PLUGIN_DELEGATE_H_