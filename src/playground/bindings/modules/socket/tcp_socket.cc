// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/logging.h"
#include "bindings/modules/socket/tcp_socket.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace bindings {
namespace socket {

namespace {

std::shared_ptr<bindings::Runtime> Runtime() {
  return bindings::Runtime::FromIsolate(v8::Isolate::GetCurrent());
}

}  // namespace

TcpSocket::TcpSocket()
    : main_thread_io_context_(Runtime()->main_thread_io_context()),
      background_io_context_(Runtime()->background_io_context()),
      boost_deadline_timer_(background_io_context_),
      boost_tcp_socket_(background_io_context_) {}

TcpSocket::~TcpSocket() = default;

void TcpSocket::CallOnMainThread(boost::function<void()> function) {
  main_thread_io_context_.post(function);
}

void TcpSocket::Open(SocketOpenOptions options, OpenCallback open_callback) {
  boost::asio::ip::tcp::endpoint endpoint{
      boost::asio::ip::address_v4::from_string(options.host),
      static_cast<uint16_t>(options.port) };

  ssl_mode_ = options.ssl;

  if (ssl_mode_ != SocketSSLMode::kNone) {
    auto method = boost::asio::ssl::context::tls;
    switch (ssl_mode_) {
      case SocketSSLMode::kAuto:
        method = boost::asio::ssl::context::sslv23;
        break;
      case SocketSSLMode::kSSL:
        method = boost::asio::ssl::context::sslv3;
        break;
      case SocketSSLMode::kTLS:
        method = boost::asio::ssl::context::tls;
        break;
      case SocketSSLMode::kTLSv11:
        method = boost::asio::ssl::context::tlsv11;
        break;
      case SocketSSLMode::kTLSv12:
        method = boost::asio::ssl::context::tlsv12;
        break;
      case SocketSSLMode::kTLSv13:
        method = boost::asio::ssl::context::tlsv13;
        break;
    }

    boost_ssl_context_ = std::make_unique<boost::asio::ssl::context>(method);
    boost_ssl_context_->set_verify_mode(boost::asio::ssl::verify_none);

    boost_ssl_socket_ =
        std::make_unique<SecureSocketType>(background_io_context_, *boost_ssl_context_);
    boost_ssl_socket_->next_layer().async_connect(
        endpoint, boost::bind(&TcpSocket::OnConnected, this, boost::asio::placeholders::error,
                              open_callback));
  } else {
    boost_tcp_socket_.async_connect(
        endpoint, boost::bind(&TcpSocket::OnConnected, this, boost::asio::placeholders::error,
                              open_callback));
  }
 
  // Regardless of security mode, set a timer for the attempt to time out.
  boost_deadline_timer_.expires_from_now(boost::posix_time::seconds(options.timeout));
  boost_deadline_timer_.async_wait(
      boost::bind(&TcpSocket::OnConnectionTimeout, this, boost::asio::placeholders::error,
                  open_callback));
}

void TcpSocket::OnConnected(const boost::system::error_code& ec,
                            OpenCallback open_callback) {
  if (ec || ssl_mode_ == SocketSSLMode::kNone) {
    boost_deadline_timer_.cancel();
    CallOnMainThread(boost::bind(open_callback, ec));
    return;
  }

  boost_ssl_socket_->async_handshake(
      boost::asio::ssl::stream_base::client,
      boost::bind(&TcpSocket::OnHandshakeCompleted, this, boost::asio::placeholders::error,
                  open_callback));
}

void TcpSocket::OnConnectionTimeout(const boost::system::error_code& ec,
                                    OpenCallback open_callback) {
  if (ec.value() == boost::asio::error::operation_aborted)
    return;  // the timer was cancelled

  // Forcefully close the socket to make sure that the active connection attempt has been stopped
  // as well. This also solves a potential issue where the Promise gets resolved twice.
  if (ssl_mode_ == SocketSSLMode::kNone)
    boost_tcp_socket_.close();
  else
    boost_ssl_socket_->lowest_layer().close();

  CallOnMainThread(boost::bind(open_callback, boost::asio::error::timed_out));
}

void TcpSocket::OnHandshakeCompleted(const boost::system::error_code& ec,
                                     OpenCallback open_callback) {
  boost_deadline_timer_.cancel();
  CallOnMainThread(boost::bind(open_callback, ec));
}

void TcpSocket::Read(ReadCallback read_callback, ErrorCallback error_callback) {
  auto callback = boost::bind(&TcpSocket::OnRead, this, read_callback, error_callback,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred);
  if (ssl_mode_ == SocketSSLMode::kNone)
    boost_tcp_socket_.async_read_some(boost::asio::buffer(read_buffer_), callback);
  else
    boost_ssl_socket_->async_read_some(boost::asio::buffer(read_buffer_), callback);
}

void TcpSocket::OnRead(ReadCallback read_callback,
                       ErrorCallback error_callback,
                       const boost::system::error_code& ec,
                       std::size_t bytes_transferred) {
  if (ec) {
    CallOnMainThread(boost::bind(error_callback, ec));
    return;
  }

  auto data = std::make_shared<std::vector<uint8_t>>();
  data->assign(&read_buffer_.front(),
               &read_buffer_.front() + bytes_transferred);

  CallOnMainThread(boost::bind(read_callback, data));
  Read(read_callback, error_callback);
}

void TcpSocket::Write(void* data, size_t bytes, WriteCallback write_callback) {
  if (active_write_) {
    write_queue_.push(boost::bind(&TcpSocket::Write, this, data, bytes, write_callback));
    return;
  }

  active_write_ = true;

  auto buffer = boost::asio::buffer(data, bytes);
  auto callback = boost::bind(&TcpSocket::OnWrite, this, write_callback,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred);

  if (ssl_mode_ == SocketSSLMode::kNone)
    boost_tcp_socket_.async_send(buffer, callback);
  else
    boost_ssl_socket_->async_write_some(buffer, callback);
}

void TcpSocket::OnWrite(WriteCallback write_callback,
                        const boost::system::error_code& ec,
                        std::size_t bytes_transferred) {
  CallOnMainThread(boost::bind(write_callback, ec, bytes_transferred));

  active_write_ = false;

  if (!write_queue_.empty()) {
    auto callback = write_queue_.front();
    write_queue_.pop();

    callback();
  }
}

void TcpSocket::Close(CloseCallback close_callback) {
  if (ssl_mode_ == SocketSSLMode::kNone) {
    boost_tcp_socket_.close();

    CallOnMainThread(close_callback);
    return;
  }

  // Shutdown of SSL sockets is asynchronous, because a TLS closing handshake has to be issued
  // and responded to by the server. We won't be able to do this when deconstructing.
  boost_ssl_socket_->async_shutdown(
      boost::bind(&TcpSocket::OnClose, this, close_callback, boost::asio::placeholders::error));
}

void TcpSocket::OnClose(CloseCallback close_callback, const boost::system::error_code& ec) {
  CallOnMainThread(close_callback);
}

}  // namespace socket
}  // namespace bindings
