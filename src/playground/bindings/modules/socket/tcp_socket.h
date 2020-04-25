// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_

#include <memory>
#include <queue>

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace bindings {
namespace socket {

// Type of secure context to establish with the socket. Only applicable to TCP sockets.
enum class Security {
  // No security should be applied over the socket.
  kNone,

  // Automatically negotiate the best option for the connection.
  kAuto,

  // The socket should only connect using any version of TLS.
  kTLS,

  // Specific algorithms that may be chosen.
  kSSLv3,
  kTLSv11,
  kTLSv12,
  kTLSv13,
};

// Implementation of the BaseSocket that provides TCP networking abilities.
class TcpSocket : public BaseSocket {
 public:
  using ReadBuffer = std::array<char, 4096>;
  using WriteBuffer = std::queue<boost::function<void()>>;
  using SecureSocketType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

  TcpSocket();
  ~TcpSocket() override;

  // BaseSocket implementation:
  bool ParseOptions(v8::Local<v8::Context> context, v8::Local<v8::Object> options) override;
  void Open(const std::string& ip, uint16_t port, int32_t timeout, OpenCallback open_callback,
            TimeoutCallback timeout_callback) override;
  void Read(ReadCallback read_callback, ErrorCallback error_callback) override;;
  void Write(void* data, size_t bytes, WriteCallback write_callback) override;
  void Close(CloseCallback close_callback) override;
  Protocol protocol() const override;

 private:
  // Called when the socket has connected. Acts as a trampoline towards the callback.
  void OnConnected(const boost::system::error_code& ec,
                   OpenCallback open_callback);

  // Called when the connection handshake has completed for secure connections.
  void OnHandshakeCompleted(const boost::system::error_code& ec,
                            OpenCallback open_callback);

  // Called when data has been received over the socket, or an error occurred.
  void OnRead(ReadCallback read_callback,
              ErrorCallback error_callback,
              const boost::system::error_code& ec,
              std::size_t bytes_transferred);

  // Called when a write operation has completed. Particularly in secured connections it's
  // important to wait for completion of one operation before starting the next, so this
  // might continue flushing the queue if any.
  void OnWrite(WriteCallback write_callback,
               const boost::system::error_code& ec,
               std::size_t bytes_transferred);

  // Called when the socket has been closed.
  void OnClose(CloseCallback close_callback, const boost::system::error_code& ec);

  // Level of security that should be applied to the socket.
  Security security_;

  // The IO Context, owned by the bindings Runtime, on which to post tasks.
  boost::asio::io_context& io_context_;

  // The SSL context to use with this socket.
  std::unique_ptr<boost::asio::ssl::context> boost_ssl_context_;
  std::unique_ptr<SecureSocketType> boost_ssl_socket_;

  // The underlying Boost socket that powers this instance.
  boost::asio::deadline_timer boost_deadline_timer_;
  boost::asio::ip::tcp::socket boost_tcp_socket_;

  // Buffer for the incoming message(s).
  ReadBuffer read_buffer_;

  // Buffer for the outgoing message(s), and a flag on whether there's a pending write.
  WriteBuffer write_buffer_;
  bool active_write_;

  DISALLOW_COPY_AND_ASSIGN(TcpSocket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
