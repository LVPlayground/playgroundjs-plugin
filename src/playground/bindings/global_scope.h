// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
#define PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/asio.hpp>
#include <include/v8.h>

#include "base/macros.h"
#include "bindings/provided_natives.h"

namespace plugin {
class PluginController;
}

namespace bindings {

class Console;
class Event;
class MySQLModule;
class PawnInvoke;
class Runtime;
class StreamerModule;

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
  explicit GlobalScope(plugin::PluginController* plugin_controller);
  ~GlobalScope();

  // Registers |event| as the interface for handling events of type |type|. All event types must
  // be registered before the InstallPrototypes() method gets called.
  void RegisterEvent(const std::string& type, std::unique_ptr<Event> event);

  // Installs the prototypes for global objects on the |global| template. This is the first of a
  // two-pass global object initialization sequence.
  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);

  // Installs instances of objects that should be available on the |context| object's global. The
  // prototypes associated with these objects must have been registered during InstallPrototypes().
  void InstallObjects(v8::Local<v8::Context> context);

  // Finalizes the Global Scope, i.e. marks the pawn function repository as read-only.
  void Finalize();

  // Accessor providing access to the instances of created event types.
  Event* GetEvent(const std::string& type);

  // Function providing access to the logstash interface.
  void logstash(const std::string& message, const std::string& socket);

  // Accessors providing access to global object instances.
  ProvidedNatives* GetProvidedNatives() { return &natives_;  }
  Console* GetConsole() { return console_.get(); }
  PawnInvoke* GetPawnInvoke() { return pawn_invoke_.get(); }

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

  // Implementation of the isPlayerMinimized() global function.
  bool IsPlayerMinimized(int player_id) const;

  // Implementation of the removeEventListener() function, which will remove |listener| from the
  // persistently held list of handlers for events of type |type|. If |listener| is an empty
  // reference, all associated listeners for events of type |type| will be removed.
  void RemoveEventListener(const std::string& type, v8::Local<v8::Function> listener);

  // Reads the contents of |filename| and returns them as a string.
  std::string ReadFile(const std::string& filename) const;

  // Returns a promise that will be resolved after |time| milliseconds.
  v8::Local<v8::Promise> Wait(Runtime* runtime, int64_t time);

 private:
  // Installs the function named |name| on the |global| template, for which the v8 engine will
  // invoke |callback| upon calls made to the function in JavaScript.
  void InstallFunction(v8::Local<v8::ObjectTemplate> global,
                       const std::string& name, v8::FunctionCallback callback);

  // Whether the global scope has been finalized.
  bool finalized_;

  // Natives that the JavaScript code provides to Pawn.
  ProvidedNatives natives_;

  // The Console object, which provides debugging abilities to authors.
  std::unique_ptr<Console> console_;

  // The PawnInvoke object, which enables authors to call Pawn native functions.
  std::unique_ptr<PawnInvoke> pawn_invoke_;

  // Weak reference to the PluginController servicing this global scope.
  plugin::PluginController* plugin_controller_;

  // The Streamer module, which enables lower-level streaming operations from JavaScript.
  std::unique_ptr<StreamerModule> streamer_module_;

  // The MySQL module, which grants access to MySQL connections from JavaScript.
  std::unique_ptr<MySQLModule> mysql_module_;

  // Map of callback names to the Event* instance that defines their interface.
  std::unordered_map<std::string, std::unique_ptr<Event>> events_;

  using v8PersistentFunctionReference = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;
  using v8PersistentFunctionVector = std::vector<v8PersistentFunctionReference>;

  // Map of event type to list of event listeners, stored as persistent references to v8 functions.
  std::unordered_map<std::string, v8PersistentFunctionVector> event_listeners_;

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
  boost::asio::io_service logstash_io_service_;
  boost::asio::local::stream_protocol::socket logstash_socket_;

  std::string logstash_socket_endpoint_;
#endif

  bool has_shown_warning_ = false;

  DISALLOW_COPY_AND_ASSIGN(GlobalScope);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_GLOBAL_SCOPE_H_
