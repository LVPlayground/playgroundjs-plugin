// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_

#include "base/macros.h"

namespace socket {

// Network protocol that should be used by a created socket.
enum class Protocol {
  kTcp,
  kUdp
};

// Represents the implementation of a Socket, separated from the JavaScript binding semantics
// that are provided by the SocketModule implementation.
class Socket {
 public:
  explicit Socket(Protocol protocol);
  ~Socket();

  // Getters to the fixed data properties of this socket.
  Protocol protocol() const { return protocol_; }

 private:
  Protocol protocol_;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

}

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
