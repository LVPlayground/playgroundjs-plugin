// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_WEB_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_WEB_SOCKET_H_

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"

namespace bindings {
namespace socket {

// Implementation of the BaseSocket that provides WebSocket networking abilities.
// https://tools.ietf.org/html/rfc6455
class WebSocket : public BaseSocket {
 public:
  WebSocket();
  ~WebSocket() override;

  // BaseSocket implementation:
  void Open(SocketOpenOptions options, OpenCallback open_callback) override;
  void Read(ReadCallback read_callback, ErrorCallback error_callback) override;;
  void Write(void* data, std::size_t bytes, WriteCallback write_callback) override;
  void Close(CloseCallback close_callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebSocket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_WEB_SOCKET_H_
