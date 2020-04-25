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

}  // namespace socket
}  // namespace bindings

