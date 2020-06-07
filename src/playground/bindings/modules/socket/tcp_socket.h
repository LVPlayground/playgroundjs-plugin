// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_

#include <memory>
#include <queue>

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"
#include "bindings/modules/socket/socket_ssl_mode.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace bindings {
namespace socket {

// Implementation of the BaseSocket that provides TCP networking abilities.
class TcpSocket : public BaseSocket {
 public:
  TcpSocket();
  ~TcpSocket() override;

  // BaseSocket implementation:
  void Open(SocketOpenOptions options, OpenCallback open_callback) override;
  void Read(ReadCallback read_callback, ErrorCallback error_callback) override;;
  void Write(void* data, std::size_t bytes, WriteCallback write_callback) override;
  void Close(CloseCallback close_callback) override;

 private:
  using ReadBuffer = std::array<char, 4096>;  
  using SecureSocketType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
  using WriteQueue = std::queue<boost::function<void()>>;

  // Schedules the given |function| to be called on the main thread. Will be aligned with SA-MP
  // server frames. This is necessary because Socket I/O is done on a background thread.
  void CallOnMainThread(boost::function<void()> function);

  // Called when DNS resolution has finished prior to opening the connection. We might now know
  // where the connection has to be opened to.
  void OnResolved(const boost::system::error_code& ec,
                  boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                  SocketOpenOptions options,
                  OpenCallback open_callback);

  // Called when the socket has connected. Sockets with a security context will start performing
  // the handshake, whereas regular, non-secure sockets are hereafter considered active.
  void OnConnected(const boost::system::error_code& ec,
                   OpenCallback open_callback);

  // Called when the socket connection has timed out. The method may be called with an |ec| of
  // operation aborted, which happens when the deadline timer gets cancelled instead.
  void OnConnectionTimeout(const boost::system::error_code& ec, OpenCallback open_callback);

  // Called when the connection handshake has completed for secure connections. For secured
  // connections, this signals the point at which the connection is ready for use.
  void OnHandshakeCompleted(const boost::system::error_code& ec, OpenCallback open_callback);

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
  SocketSSLMode ssl_mode_ = SocketSSLMode::kNone;

  // The IO Contexts, owned by the bindings Runtime, on which to post tasks.
  boost::asio::io_context& main_thread_io_context_;
  boost::asio::io_context& background_io_context_;

  // The resolver used for resolving DNS, when used rather than an IP address.
  boost::asio::ip::tcp::resolver resolver_;

  // The SSL context to use with this socket.
  std::unique_ptr<boost::asio::ssl::context> boost_ssl_context_;
  std::unique_ptr<SecureSocketType> boost_ssl_socket_;

  // The underlying Boost socket that powers this instance.
  boost::asio::deadline_timer boost_deadline_timer_;
  boost::asio::ip::tcp::socket boost_tcp_socket_;

  // Buffer for the incoming message(s).
  ReadBuffer read_buffer_;

  // Queue for the outgoing message(s), and a flag on whether there's a pending write.
  WriteQueue write_queue_;
  bool active_write_ = false;

  DISALLOW_COPY_AND_ASSIGN(TcpSocket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_TCP_SOCKET_H_
