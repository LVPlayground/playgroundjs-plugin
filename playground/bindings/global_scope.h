// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
#define PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include <include/v8.h>

#include "bindings/global_scope_event_target.h"

namespace plugin {
class Arguments;
struct Callback;
}

namespace bindings {

class Console;
class Event;
class Include;
class Runtime;

// The global scope defines the globally available interfaces and properties to the JavaScript
// code being executed by this plugin. The objects are being installed in two steps: firstly the
// prototypes, which will be created on an ObjectTemplate before the context exists, and then
// the objects, which will be created once a context has been created.
//
// Finally, a "self" member will be created in the global scope as well, which allows Web Worker-
// like access to the properties on the global scope.
class GlobalScope : public GlobalScopeEventTarget {
 public:
  explicit GlobalScope(Runtime* runtime);
  ~GlobalScope();

  // Creates a JavaScript event interface for the Pawn |callback|.
  void CreateInterfaceForCallback(const plugin::Callback& callback);

  // Creates a JavaScript event object for the Pawn |callback|, and populates it with |arguments|.
  v8::Local<v8::Object> CreateEventForCallback(const plugin::Callback& callback,
                                               const plugin::Arguments& arguments);

  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);
  void InstallObjects(v8::Local<v8::Object> global);

 private:
  // Weak, owns us. The runtime we service the global object for.
  Runtime* runtime_;

  // The Console object, which provides debugging abilities to authors.
  std::unique_ptr<Console> console_;

  // The Include object, which provides the global include() function.
  std::unique_ptr<Include> include_;

  // Map of callback names to the Event* instance that defines their interface.
  std::unordered_map<std::string, std::unique_ptr<Event>> events_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
