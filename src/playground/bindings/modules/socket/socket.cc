// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket.h"

namespace socket {

Socket::Socket(Protocol protocol)
    : protocol_(protocol),
      state_(State::kDisconnected) {}

Socket::~Socket() = default;

void Socket::open(const std::string& ip, int32_t port, int32_t timeout, std::unique_ptr<bindings::Promise> promise) {
  promise->Resolve(true);
}

void Socket::close() {

}

}  // namespace
