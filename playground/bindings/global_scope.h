// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
#define PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include <include/v8.h>

#include "base/macros.h"

namespace bindings {

class Console;
class Event;
class Runtime;

// The global scope object represents and owns the global scope of the Runtime instance that owns
// it. It will own the global objects and events, instances of said objects and event listeners.
//
// Installation of a global scope has to be done in two steps. First the prototypes of global
// interfaces will have to be installed on the object template of the global, which is done by the
// InstallPrototypes() method. Then the global itself must be initialized, after which the
// InstallObjects() method should be called on the created object, so that references and instances
// can be defined as desired.
//
// Because the global scope holds persistent references to event listeners, it is important that
// it gets deleted before the v8 context or isolate. Failing to do so will result in a SEGFAULT.
class GlobalScope {
 public:
  GlobalScope();
  ~GlobalScope();

  // Registers |event| as the interface for handling events of type |type|. All event types must
  // be registered before the InstallPrototypes() method gets called.
  void RegisterEvent(const std::string& type, std::unique_ptr<Event> event);

  // Installs the prototypes for global objects on the |global| template. This is the first of a
  // two-pass global object initialization sequence.
  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);

  // Installs instances of objects that should be available on the |global| object. The prototypes
  // associated with these objects must have been registered during InstallPrototypes().
  void InstallObjects(v8::Local<v8::Object> global);

  // Accessor providing access to the instances of created event types.
  Event* GetEvent(const std::string& type);

  // Accessor providing access to the global Console instance.
  Console* GetConsole() { return console_.get(); }

 public:
  // Implementation of the addEventListener() function, which registers |listener| as a handler
  // for events of type |type|. A persistent reference to |listener| will be created.
  void AddEventListener(const std::string& type, v8::Local<v8::Function> listener);

  // Implementation of the dispatchEvent() function, which will invoke all listeners registered for
  // events of type |type|. The |event| value will be re-used for each invocation.
  bool DispatchEvent(const std::string& type, v8::Local<v8::Value> event) const;

  // Implementation of the hasEventListeners() function, which returns whether there are any
  // registered event listeners for events of type |type|.
  bool HasEventListeners(const std::string& type) const;

  // Implementation of the highResolutionTime() global function, which will return a timing value
  // with sub-millisecond precision.
  double HighResolutionTime() const;

  // Implementation of the removeEventListener() function, which will remove |listener| from the
  // persistently held list of handlers for events of type |type|. If |listener| is an empty
  // reference, all associated listeners for events of type |type| will be removed.
  void RemoveEventListener(const std::string& type, v8::Local<v8::Function> listener);

  // Implementation of the requireImpl() global function, which loads and executes |filename| with
  // a CommonJS-esque module boilerplate in the current global scope and returns the result.
  v8::Local<v8::Value> RequireImpl(Runtime* runtime, const std::string& filename) const;

 private:
  // Installs the function named |name| on the |global| template, for which the v8 engine will
  // invoke |callback| upon calls made to the function in JavaScript.
  void InstallFunction(v8::Local<v8::ObjectTemplate> global,
                       const std::string& name, v8::FunctionCallback callback);

  // The Console object, which provides debugging abilities to authors.
  std::unique_ptr<Console> console_;

  // Map of callback names to the Event* instance that defines their interface.
  std::unordered_map<std::string, std::unique_ptr<Event>> events_;

  using v8PersistentFunctionReference = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;
  using v8PersistentFunctionVector = std::vector<v8PersistentFunctionReference>;

  // Map of event type to list of event listeners, stored as persistent references to v8 functions.
  std::unordered_map<std::string, v8PersistentFunctionVector> event_listeners_;

  DISALLOW_COPY_AND_ASSIGN(GlobalScope);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
