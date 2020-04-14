// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket.h"

namespace socket {

Socket::Socket(Protocol protocol)
  : protocol_(protocol) {}

Socket::~Socket() = default;

}  // namespace
