// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_WEB_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_WEB_SOCKET_H_

#include "base/macros.h"
#include "bindings/modules/socket/base_socket.h"
#include "bindings/modules/socket/socket_ssl_mode.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

namespace bindings {
namespace socket {

// Implementation of the BaseSocket that provides WebSocket networking abilities.
// https://tools.ietf.org/html/rfc6455
class WebSocket : public BaseSocket {
 public:
  WebSocket();
  ~WebSocket() override;

  // BaseSocket implementation:
  void Open(SocketOpenOptions options, OpenCallback open_callback) override;
  void Read(ReadCallback read_callback, ErrorCallback error_callback) override;;
  void Write(void* data, std::size_t bytes, WriteCallback write_callback) override;
  void Close(CloseCallback close_callback) override;

 private:
  using WsSocketType = boost::beast::websocket::stream<boost::beast::tcp_stream>;
  using WssSocketType =
      boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

  // Schedules the given |function| to be called on the main thread. Will be aligned with SA-MP
  // server frames. This is necessary because Socket I/O is done on a background thread.
  void CallOnMainThread(boost::function<void()> function);

  // Called when DNS resolution has finished prior to opening the connection. We might now know
  // where the connection has to be opened to.
  void OnResolved(const boost::system::error_code& ec,
                  boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                  SocketOpenOptions options,
                  OpenCallback open_callback);

  // Called when the secure socket has connected. Will continue with the SSL handshake to make
  // sure that we can talk to the remote host in a secure manner.
  void OnSecureConnected(const boost::system::error_code& ec,
                         SocketOpenOptions options,
                         OpenCallback open_callback);

  // Called when a secure connection has been established. Proceeds with the WebSocket handshake
  // that will issue the HTTP Upgrade call through headers.
  void OnConnected(const boost::system::error_code& ec,
                   SocketOpenOptions options,
                   OpenCallback open_callback);

  // Called when the connection handshake has completed for secure connections. For secured
  // connections, this signals the point at which the connection is ready for use.
  void OnHandshakeCompleted(const boost::system::error_code& ec, OpenCallback open_callback);

  // Called when the socket connection has timed out. The method may be called with an |ec| of
  // operation aborted, which happens when the deadline timer gets cancelled instead.
  void OnConnectionTimeout(const boost::system::error_code& ec, OpenCallback open_callback);

  // Level of security that should be applied to the socket.
  SocketSSLMode ssl_mode_ = SocketSSLMode::kNone;

  // The IO Contexts, owned by the bindings Runtime, on which to post tasks.
  boost::asio::io_context& main_thread_io_context_;
  boost::asio::io_context& background_io_context_;

  // The resolver used for resolving DNS, when used rather than an IP address.
  boost::asio::ip::tcp::resolver resolver_;

  // The SSL context to use with this socket.
  std::unique_ptr<boost::asio::ssl::context> boost_ssl_context_;

  // The deadline timer for managing connection timeouts.
  boost::asio::deadline_timer boost_deadline_timer_;

  // The stream that will be used. Depends on |ssl_mode_| to switch between ws and wss.
  std::unique_ptr<WsSocketType> ws_stream_;
  std::unique_ptr<WssSocketType> wss_stream_;

  DISALLOW_COPY_AND_ASSIGN(WebSocket);
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_WEB_SOCKET_H_
