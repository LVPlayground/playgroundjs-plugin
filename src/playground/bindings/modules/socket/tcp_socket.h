// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_

#include "bindings/modules/socket/base_socket.h"

namespace bindings {
namespace socket {

// Type of secure context to establish with the socket. Only applicable to TCP sockets.
enum class Security {
  // No security should be applied over the socket.
  kNone,

  // The socket should automatically negotiate the security layer.
  kAuto,

  // More specific security contexts that can be instantiated.
  kSSL,
  kSSLv3,
  kTLS,
  kTLSv1,
  kTLSv11,
  kTLSv12,
  kTLSv13,
};

// Implementation of the BaseSocket that provides TCP networking abilities.
class TcpSocket : public BaseSocket {
 public:
  TcpSocket();
  ~TcpSocket() override;

  // BaseSocket implementation:
  bool ParseOptions(const v8::Local<v8::Object>& options) override;
  Protocol protocol() const override;

 private:
  Security security_;
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
