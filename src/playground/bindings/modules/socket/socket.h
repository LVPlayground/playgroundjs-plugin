// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_

#include <array>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"
#include "bindings/promise.h"

namespace bindings {
namespace socket {

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
  class SocketObserver {
   public:
    virtual void OnClose() = 0;
    virtual void OnError(int code, const std::string& message) = 0;
    virtual void OnMessage(void* data, std::size_t bytes) = 0;
  };

  Socket(std::unique_ptr<BaseSocket> engine, SocketObserver* observer);
  ~Socket();

  // Opens a connection with the given parameters. A promise is included that should be resolved
  // when the connection either has been established, failed or timed out.
  void Open(const std::string& ip, uint16_t port, int32_t timeout,
            std::unique_ptr<Promise> promise);

  // Writes the given |data| (of length |bytes|) to the socket. The given |promise| will be resolved
  // with a boolean on whether the write operation has succeeded.
  void Write(uint8_t* data, size_t bytes, std::unique_ptr<Promise> promise);

  // Immediately closes the connection currently held by this socket.
  void Close(std::shared_ptr<Promise> promise);

  // Returns whether the socket can be closed, depending on its current state.
  bool CanClose() const {
    return state_ == State::kConnecting ||
           state_ == State::kConnected;
  }

  // Getters to the fixed data properties of this socket.
  BaseSocket* engine() const { return engine_.get(); }
  State state() const { return state_; }

 private:
  class WriteData {
   public:
    WriteData(std::unique_ptr<Promise> promise, std::unique_ptr<std::vector<uint8_t>> buffer)
        : promise(std::move(promise)), buffer(std::move(buffer)) {}
    ~WriteData() = default;

    std::unique_ptr<Promise> promise;
    std::unique_ptr<std::vector<uint8_t>> buffer;

   private:
    DISALLOW_COPY_AND_ASSIGN(WriteData);
  };

  // To be called by Boost when the connection attempt has a result.
  void OnConnect(const boost::system::error_code& ec);

  // To be called by Boost when the connection timeout timer has fired.
  void OnConnectTimeout(const boost::system::error_code& ec);

  // To be called by Boost when data has been received over the socket.
  void OnRead(void* data, std::size_t bytes);

  // To be called by Boost when a data reading error has occurred.
  void OnError(const boost::system::error_code& ec);

  // To be called by Boost when data has been written to the socket.
  void OnWrite(const boost::system::error_code& ec,
               std::size_t bytes_transferred,
               std::shared_ptr<WriteData> write_data);

  // To be called by Boost when the connection has been closed.
  void OnClose(std::shared_ptr<Promise> promise);

  // The engine that powers this socket connection.
  std::unique_ptr<BaseSocket> engine_;

  State state_;
  SocketObserver* observer_;

  // The connection promise that is pending for the current connection attempt.
  std::unique_ptr<Promise> connection_promise_;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_SOCKET_H_
