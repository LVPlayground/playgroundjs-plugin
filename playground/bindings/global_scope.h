// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
#define PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include <include/v8.h>

namespace plugin {
class Arguments;
struct Callback;
}

namespace bindings {

class Console;
class Event;
class Runtime;

// The global scope defines the globally available interfaces and properties to the JavaScript
// code being executed by this plugin. The objects are being installed in two steps: firstly the
// prototypes, which will be created on an ObjectTemplate before the context exists, and then
// the objects, which will be created once a context has been created.
//
// Finally, a "self" member will be created in the global scope as well, which allows Web Worker-
// like access to the properties on the global scope.
class GlobalScope {
 public:
  explicit GlobalScope(Runtime* runtime);
  ~GlobalScope();

  // Creates a JavaScript event interface for the Pawn |callback|.
  void CreateInterfaceForCallback(const plugin::Callback& callback);

  // Creates a JavaScript event object for the Pawn |callback|, and populates it with |arguments|.
  v8::Local<v8::Object> CreateEventForCallback(const plugin::Callback& callback,
                                               const plugin::Arguments& arguments);

  // JavaScript functions for changing the available event listeners.
  void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments);
  void removeEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments);
  bool triggerEvent(const std::string& event_name, v8::Local<v8::Object> object);

  inline bool hasEventListeners(const std::string& event_name) const {
    return event_listeners_.find(event_name) != event_listeners_.end();
  }

  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);
  void InstallObjects(v8::Local<v8::Object> global);

  void Dispose();

 private:
  // Weak, owns us. The runtime we service the global object for.
  Runtime* runtime_;

  // The Console object, which provides debugging abilities to authors.
  std::unique_ptr<Console> console_;

  // Map of callback names to the Event* instance that defines their interface.
  std::unordered_map<std::string, std::unique_ptr<Event>> events_;

  // Map of event name to a vector of persistent handles to the registered function handlers.
  using PersistentFunction = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;
  using PersistentFunctionVector = std::vector<PersistentFunction>;

  std::unordered_map<std::string, PersistentFunctionVector> event_listeners_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
