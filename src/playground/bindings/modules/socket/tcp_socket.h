// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"

#include <boost/asio.hpp>

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
  using ReadBuffer = std::array<char, 4096>;

  TcpSocket();
  ~TcpSocket() override;

  // BaseSocket implementation:
  bool ParseOptions(const v8::Local<v8::Object>& options) override;
  void Open(const std::string& ip, uint16_t port, int32_t timeout, OpenCallback open_callback,
            TimeoutCallback timeout_callback) override;
  void Read(ReadCallback read_callback, ErrorCallback error_callback) override;;
  void Write(void* data, size_t bytes, WriteCallback write_callback) override;
  void Close() override;
  Protocol protocol() const override;

 private:
  // Called when data has been received over the socket, or an error occurred.
  void OnRead(ReadCallback read_callback,
              ErrorCallback error_callback,
              const boost::system::error_code& ec,
              std::size_t bytes_transferred);

  // The IO Context, owned by the bindings Runtime, on which to post tasks.
  boost::asio::io_context& io_context_;

  // The underlying Boost socket that powers this instance.
  boost::asio::deadline_timer boost_deadline_timer_;
  boost::asio::ip::tcp::socket boost_tcp_socket_;

  // Buffer for the incoming message(s).
  ReadBuffer read_buffer_;

  DISALLOW_COPY_AND_ASSIGN(TcpSocket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
