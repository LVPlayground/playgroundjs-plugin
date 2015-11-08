// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_CALLBACK_HOOK_H_
#define PLAYGROUND_PLUGIN_CALLBACK_HOOK_H_

#include <memory>
#include <string>
#include <unordered_map>

typedef struct tagAMX AMX;

class SubHook;

namespace plugin {

class Arguments;
struct Callback;
class CallbackParser;

// The callback hook overrides a number of natives from the Pawn runtime in order to intercept
// methods being called by the SA-MP server, based on a list of parsed callbacks. Intercepted
// calls will then be forwarded to a dedicated delegate class.
class CallbackHook {
 public:
  // The delegate class defines the interface that must be supported by the object that wishes
  // to be invoked upon both availability of the gamemode's AMX structure, and invoked callbacks.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when the gamemode loaded in the Pawn runtime has changed.
    virtual void OnGamemodeChanged(AMX* gamemode) = 0;

    // Called when a callback to the gamemode has been intercepted. Returning true will block
    // the callback from being invoked in the Pawn runtime.
    virtual bool OnCallbackIntercepted(const std::string& callback, const Arguments& arguments) = 0;
  };

  // Place this on the stack to ignore interceptable callbacks until it goes out of scope.
  class ScopedIgnore {
   public:
    ScopedIgnore();
    ~ScopedIgnore();
  };

  CallbackHook(Delegate* delegate, const std::shared_ptr<CallbackParser>& callback_parser);
  ~CallbackHook();

  // Installs the callback hook in the SA-MP server. Returns whether installation was successful.
  bool Install();

  // Called when Pawn code is being executed by the SA-MP server. The return value should be
  // one of Pawn's error codes (most likely AMX_ERR_NONE).
  int OnExecute(AMX* amx, int* retval, int index);

 private:
  // Called when the primary gamemode is being initialized. This provides us with an appropriate
  // time to cache the indices of callbacks that should be intercepted.
  void OnGamemodeLoaded(AMX* amx);

  // Called when |callback| has been intercepted by the |amx| runtime. Returning true will block
  // the callback from being invoked on the Pawn runtime, returning |return_value| there.
  bool DoIntercept(AMX* amx, int* retval, const Callback& callback);

  // Weak reference. Will usually own this instance.
  Delegate* delegate_;

  // The callback parser that has knowledge of the callbacks to intercept.
  std::shared_ptr<CallbackParser> callback_parser_;

  // The hook which will be installed in the SA-MP server's memory.
  std::unique_ptr<SubHook> hook_;

  // Mapping of function indices to the callback format that is to be intercepted.
  std::unordered_map<int, const Callback*> intercept_indices_;

  AMX* gamemode_;
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_CALLBACK_HOOK_H_
