// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_

#include <memory>
#include <stdint.h>
#include <string>

#include "base/macros.h"
#include "bindings/promise.h"

namespace socket {

// Network protocol that should be used by a created socket.
enum class Protocol {
  kTcp,
  kUdp
};

// State of the socket. Its behaviour is asynchronous, so there are transitional states.
enum class State {
  kConnecting,
  kConnected,
  kDisconnected
};

// Represents the implementation of a Socket, separated from the JavaScript binding semantics
// that are provided by the SocketModule implementation.
class Socket {
 public:
  explicit Socket(Protocol protocol);
  ~Socket();

  // Opens a connection with the given parameters. A promise is included that should be resolved
  // when the connection either has been established, failed or timed out.
  void open(const std::string& ip, int32_t port, int32_t timeout, std::unique_ptr<bindings::Promise> promise);

  // Immediately closes the connection currently held by this socket.
  void close();

  // Returns whether the socket can be closed, depending on its current state.
  bool canClose() const {
    return state_ == State::kConnecting ||
           state_ == State::kConnected;
  }

  // Getters to the fixed data properties of this socket.
  Protocol protocol() const { return protocol_; }
  State state() const { return state_; }

 private:
  Protocol protocol_;
  State state_;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

}

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
