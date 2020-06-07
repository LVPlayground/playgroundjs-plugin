// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/web_socket.h"

namespace bindings {
namespace socket {

WebSocket::WebSocket() = default;

WebSocket::~WebSocket() = default;

void WebSocket::Open(SocketOpenOptions options, OpenCallback open_callback) {}

void WebSocket::Read(ReadCallback read_callback, ErrorCallback error_callback) {}

void WebSocket::Write(void* data, std::size_t bytes, WriteCallback write_callback) {}

void WebSocket::Close(CloseCallback close_callback) {}

}  // namespace socket
}  // namespace bindings
