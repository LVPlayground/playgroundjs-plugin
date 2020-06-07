// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_PROTOCOL_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_PROTOCOL_H_

#include <include/v8.h>

namespace bindings {
namespace socket {

enum class SocketProtocol {
  kTCP,
  kWebSocket
};

bool ParseSocketProtocol(v8::Local<v8::Value> value, SocketProtocol* protocol);

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_PROTOCOL_H_
