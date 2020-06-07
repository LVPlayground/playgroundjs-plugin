// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket_protocol.h"

#include "base/logging.h"
#include "bindings/utilities.h"

namespace bindings {
namespace socket {

bool ParseSocketProtocol(v8::Local<v8::Value> value, SocketProtocol* protocol) {
  CHECK(protocol);

  if (!value->IsString())
    return false;

  std::string string = toString(value);

  if (string == "tcp")
    *protocol = SocketProtocol::kTCP;
  else if (string == "websocket")
    *protocol = SocketProtocol::kWebSocket;
  else
    return false;

  return true;
}

}  // namespace socket
}  // namespace bindings
