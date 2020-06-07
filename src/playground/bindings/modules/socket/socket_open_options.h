// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_OPEN_OPTIONS_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_OPEN_OPTIONS_H_

#include <stdint.h>
#include <string>

#include "bindings/modules/socket/socket_ssl_mode.h"

#include <include/v8.h>

namespace bindings {
namespace socket {

struct SocketOpenOptions {
  SocketOpenOptions()
      : port(-1), timeout(30), ssl(SocketSSLMode::kNone) {}

  SocketOpenOptions(SocketOpenOptions&&) = default;
  SocketOpenOptions(const SocketOpenOptions&) = default;

  SocketOpenOptions& operator=(SocketOpenOptions&&) = default;
  SocketOpenOptions& operator=(SocketOpenOptions const&) = default;

  std::string host;
  int32_t port;
  int32_t timeout;
  SocketSSLMode ssl;
};

bool ParseSocketOpenOptions(v8::Local<v8::Value> value, SocketOpenOptions* options);

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_OPEN_OPTIONS_H_
