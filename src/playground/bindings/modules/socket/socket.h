// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_

#include <cstddef>
#include <memory>
#include <string>

#include <boost/system/system_error.hpp>

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"
#include "bindings/promise.h"

namespace bindings {
namespace socket {

struct SocketOpenOptions;

// State of the socket. Its behaviour is asynchronous, so there are transitional states.
enum class State {
  kConnecting,
  kConnected,
  kDisconnecting,
  kDisconnected
};

// Represents the implementation of a Socket, separated from the JavaScript binding semantics
// that are provided by the SocketModule implementation. An observer is used to inform the
// JavaScript bindings of events that happen during the socket's lifetime.
class Socket {
 public:
  class SocketObserver {
   public:
    virtual void OnClose() = 0;
    virtual void OnError(int code, const std::string& message) = 0;
    virtual void OnMessage(void* data, std::size_t bytes) = 0;

   protected:
    virtual ~SocketObserver() = default;
  };

  Socket(std::unique_ptr<BaseSocket> engine, SocketObserver* observer);
  ~Socket();

  // Opens a connection with the given options. A promise is included that should be resolved
  // when the connection either has been established, failed or timed out.
  void Open(SocketOpenOptions options, std::shared_ptr<Promise> promise);

  // Writes the given |data| (of length |bytes|) to the socket. The given |promise| will be resolved
  // with a boolean on whether the write operation has succeeded.
  void Write(uint8_t* data, size_t bytes, std::shared_ptr<Promise> promise);

  // Immediately closes the connection currently held by this socket.
  void Close(std::shared_ptr<Promise> promise);

  // Getters to the fixed data properties of this socket.
  State state() const { return state_; }

 private:
  using WriteBuffer = std::shared_ptr<std::vector<uint8_t>>;

  // Called when the result of a connection attempt is known.
  void OnConnect(const boost::system::error_code& ec, std::shared_ptr<Promise> promise);

  // Called when a write operation on the socket has completed. The |buffer| is included in order
  // to keep the memory backing the write buffer alive during the operation.
  void OnWrite(const boost::system::error_code& ec,
               std::size_t bytes_transferred,
               std::shared_ptr<Promise> promise,
               WriteBuffer buffer);

  // Called when the connection has been successfully closed.
  void OnClose(std::shared_ptr<Promise> promise);

  // Called when data has been received from the socket.
  void OnRead(std::shared_ptr<std::vector<uint8_t>> data);

  // Called when an error has occurred on the socket.
  void OnError(const boost::system::error_code& ec);

  // The engine that powers this socket connection.
  std::unique_ptr<BaseSocket> engine_;

  State state_;
  SocketObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
