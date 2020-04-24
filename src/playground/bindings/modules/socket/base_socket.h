// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_

#include <include/v8.h>

namespace bindings {
namespace socket {

// Network protocol that should be used by a created socket.
enum class Protocol {
  kTcp
};

// Base class for all socket operations. Methods are implemented by the specific socket types, for
// example the TcpSocket class.
class BaseSocket {
 public:
  virtual ~BaseSocket() {}

  // Parses the given |options|, which are specific to the kind of socket. State should be stared
  // locally in socket, but does not have to be reflected on the object later on.
  virtual bool ParseOptions(const v8::Local<v8::Object>& options) = 0;

  // Returns the protocol that this socket engine implements.
  virtual socket::Protocol protocol() const = 0;
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_
