// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket_ssl_mode.h"

#include "base/logging.h"

namespace bindings {
namespace socket {

bool FromString(const std::string& string, SocketSSLMode* mode) {
  CHECK(mode);

  if (string == "none")
    *mode = SocketSSLMode::kNone;
  else if (string == "auto")
    *mode = SocketSSLMode::kAuto;
  else if (string == "ssl")
    *mode = SocketSSLMode::kSSL;
  else if (string == "tls")
    *mode = SocketSSLMode::kTLS;
  else if (string == "tlsv11")
    *mode = SocketSSLMode::kTLSv11;
  else if (string == "tlsv12")
    *mode = SocketSSLMode::kTLSv12;
  else if (string == "tlsv13")
    *mode = SocketSSLMode::kTLSv13;
  else
    return false;
  
  return true;
}

std::unique_ptr<boost::asio::ssl::context> CreateSecureContext(SocketSSLMode mode) {
  auto method = boost::asio::ssl::context::tls;
  switch (mode) {
    case SocketSSLMode::kAuto:
      method = boost::asio::ssl::context::sslv23;
      break;
    case SocketSSLMode::kSSL:
      method = boost::asio::ssl::context::sslv3;
      break;
    case SocketSSLMode::kTLS:
      method = boost::asio::ssl::context::tls;
      break;
    case SocketSSLMode::kTLSv11:
      method = boost::asio::ssl::context::tlsv11;
      break;
    case SocketSSLMode::kTLSv12:
      method = boost::asio::ssl::context::tlsv12;
      break;
    case SocketSSLMode::kTLSv13:
      method = boost::asio::ssl::context::tlsv13;
      break;
  }

  auto context = std::make_unique<boost::asio::ssl::context>(method);
  context->set_verify_mode(boost::asio::ssl::verify_none);

  return context;
}

}  // namespace socket
}  // namespace bindings

