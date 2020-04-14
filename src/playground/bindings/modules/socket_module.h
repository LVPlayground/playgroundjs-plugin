// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_MODULE_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_MODULE_H_

#include <include/v8.h>

namespace bindings {

// The Socket module allows JavaScript to open UDP and TCP sockets with arbitrary destinations,
// for example to open (and maintain) echo or IRC connections. The class is implemented on top of
// the Boost ASIO library.
//
// The |protocol| has to be either "TCP" or "UDP", insensitive to casing.

// [Constructor(string protocol)]
// interface Socket {
//     readonly attribute string protocol;
// }
class SocketModule {
 public:
  SocketModule();
  ~SocketModule();

  // Called when the prototype for the global object is being constructed. 
  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_MODULE_H_
