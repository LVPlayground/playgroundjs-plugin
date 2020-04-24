// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/tcp_socket.h"

namespace bindings {
namespace socket {

TcpSocket::TcpSocket()
    : security_(Security::kNone) {}

TcpSocket::~TcpSocket() = default;

bool TcpSocket::ParseOptions(const v8::Local<v8::Object>& options) {
  return true;
}

Protocol TcpSocket::protocol() const {
  return Protocol::kTcp;
}

}  // namespace socket
}  // namespace bindings
