// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_

#include <memory>
#include <stdint.h>
#include <string>

#include <boost/asio.hpp>

#include "base/macros.h"
#include "bindings/promise.h"

namespace bindings {
namespace socket {

// Network protocol that should be used by a created socket.
enum class Protocol {
  kTcp,
  kUdp
};

// State of the socket. Its behaviour is asynchronous, so there are transitional states.
enum class State {
  kConnecting,
  kConnected,
  kDisconnected
};

// Represents the implementation of a Socket, separated from the JavaScript binding semantics
// that are provided by the SocketModule implementation.
class Socket {
 public:
  explicit Socket(Protocol protocol);
  ~Socket();

  // Opens a connection with the given parameters. A promise is included that should be resolved
  // when the connection either has been established, failed or timed out.
  void Open(const std::string& ip, uint16_t port, int32_t timeout, std::unique_ptr<Promise> promise);

  // Immediately closes the connection currently held by this socket.
  void Close();

  // Returns whether the socket can be closed, depending on its current state.
  bool CanClose() const {
    return state_ == State::kConnecting ||
           state_ == State::kConnected;
  }

  // Getters to the fixed data properties of this socket.
  Protocol protocol() const { return protocol_; }
  State state() const { return state_; }

 private:
  // To be called by Boost when the connection attempt has a result.
  void OnConnect(const std::string& ip, uint16_t port, const boost::system::error_code& ec);

  // To be called by Boost when the connection timeout timer has fired.
  void OnConnectTimeout(const std::string& ip, uint16_t port);

  Protocol protocol_;
  State state_;

  // Global counter indicating the current connection Id.
  int64_t connection_id_;

  // The connection promise that is pending for the current connection attempt.
  std::unique_ptr<Promise> connection_promise_;

  // The IO Context, owned by the bindings Runtime, on which to post tasks.
  boost::asio::io_context& io_context_;

  // The underlying Boost socket that powers this instance.
  boost::asio::deadline_timer boost_deadline_timer_;
  boost::asio::ip::tcp::socket boost_socket_;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
