// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_SSL_MODE_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_SSL_MODE_H_

#include <memory>
#include <string>

#include <boost/asio/ssl.hpp>

namespace bindings {
namespace socket {

enum class SocketSSLMode {
  kNone,
  kAuto,
  kSSL,
  kTLS,
  kTLSv11,
  kTLSv12,
  kTLSv13
};

bool FromString(const std::string& string, SocketSSLMode* mode);

std::unique_ptr<boost::asio::ssl::context> CreateSecureContext(SocketSSLMode mode);

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_SSL_MODE_H_
