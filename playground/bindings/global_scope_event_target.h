// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_SCOPE_EVENT_TARGET_H_
#define PLAYGROUND_BINDINGS_GLOBAL_SCOPE_EVENT_TARGET_H_

#include <string>
#include <unordered_map>
#include <vector>

#include <include/v8.h>

namespace bindings {

class Runtime;

class GlobalScopeEventTarget {
 public:
  explicit GlobalScopeEventTarget(Runtime* runtime);
  ~GlobalScopeEventTarget();

  // JavaScript functions for changing the available event listeners.
  void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments);
  void removeEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments);
  bool triggerEvent(const std::string& event_name, v8::Local<v8::Object> object);

  inline bool hasEventListeners(const std::string& event_name) const {
    return event_listeners_.find(event_name) != event_listeners_.end();
  }

 private:
  Runtime* runtime_;

  // Map of event name to a vector of persistent handles to the registered function handlers.
  using PersistentFunction = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;
  using PersistentFunctionVector = std::vector<PersistentFunction>;

  std::unordered_map<std::string, PersistentFunctionVector> event_listeners_;
};

void AddEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void RemoveEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_GLOBAL_SCOPE_EVENT_TARGET_H_
