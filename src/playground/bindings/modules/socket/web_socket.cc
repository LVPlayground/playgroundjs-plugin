// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/web_socket.h"

#include "base/logging.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {
namespace socket {

namespace {

std::shared_ptr<bindings::Runtime> Runtime() {
  return bindings::Runtime::FromIsolate(v8::Isolate::GetCurrent());
}

}  // namespace

WebSocket::WebSocket()
    : main_thread_io_context_(Runtime()->main_thread_io_context()),
      background_io_context_(Runtime()->background_io_context()),
      resolver_(background_io_context_),
      boost_deadline_timer_(background_io_context_) {}

WebSocket::~WebSocket() = default;

void WebSocket::CallOnMainThread(boost::function<void()> function) {
  main_thread_io_context_.post(function);
}

void WebSocket::Open(SocketOpenOptions options, OpenCallback open_callback) {
  boost::asio::ip::tcp::resolver::query query(options.host, std::to_string(options.port));

  resolver_.async_resolve(
    query, boost::bind(&WebSocket::OnResolved, this, boost::asio::placeholders::error,
                       boost::asio::placeholders::iterator, options, open_callback));
}

void WebSocket::OnResolved(const boost::system::error_code& ec,
                           boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                           SocketOpenOptions options,
                           OpenCallback open_callback) {
  if (ec) {
    CallOnMainThread(boost::bind(open_callback, ec));
    return;
  }

  boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;

  ssl_mode_ = options.ssl;
  if (ssl_mode_ != SocketSSLMode::kNone) {
    boost_ssl_context_ = CreateSecureContext(ssl_mode_);

    wss_stream_ = std::make_unique<WssSocketType>(background_io_context_, *boost_ssl_context_);
    wss_stream_->next_layer().next_layer().async_connect(
        endpoint, boost::bind(&WebSocket::OnSecureConnected, this, boost::asio::placeholders::error,
                              options, open_callback));
  } else {
    ws_stream_ = std::make_unique<WsSocketType>(background_io_context_);
    ws_stream_->next_layer().async_connect(
        endpoint, boost::bind(&WebSocket::OnConnected, this, boost::asio::placeholders::error,
                              options, open_callback));
  }

  // Regardless of security mode, set a timer for the attempt to time out.
  boost_deadline_timer_.expires_from_now(boost::posix_time::seconds(options.timeout));
  boost_deadline_timer_.async_wait(
      boost::bind(&WebSocket::OnConnectionTimeout, this, boost::asio::placeholders::error,
                  open_callback));
}

void WebSocket::OnSecureConnected(const boost::system::error_code& ec,
                                  SocketOpenOptions options,
                                  OpenCallback open_callback) {
  if (ec) {
    boost_deadline_timer_.cancel();
    CallOnMainThread(boost::bind(open_callback, ec));
    return;
  }

  wss_stream_->next_layer().async_handshake(
      boost::asio::ssl::stream_base::client,
      boost::bind(&WebSocket::OnConnected, this, boost::asio::placeholders::error, options,
                  open_callback));
}

void WebSocket::OnConnected(const boost::system::error_code& ec,
                            SocketOpenOptions options,
                            OpenCallback open_callback) {
  if (ec) {
    boost_deadline_timer_.cancel();
    CallOnMainThread(boost::bind(open_callback, ec));
    return;
  }

  if (ssl_mode_ != SocketSSLMode::kNone) {
    wss_stream_->async_handshake(
        options.host, options.path,
        boost::bind(&WebSocket::OnHandshakeCompleted, this, boost::asio::placeholders::error,
                    open_callback));
  } else {
    ws_stream_->async_handshake(
        options.host, options.path,
        boost::bind(&WebSocket::OnHandshakeCompleted, this, boost::asio::placeholders::error,
                    open_callback));
  }
}

void WebSocket::OnHandshakeCompleted(const boost::system::error_code& ec,
  OpenCallback open_callback) {
  boost_deadline_timer_.cancel();
  CallOnMainThread(boost::bind(open_callback, ec));
}

void WebSocket::OnConnectionTimeout(const boost::system::error_code& ec,
                                    OpenCallback open_callback) {
  if (ec.value() == boost::asio::error::operation_aborted)
    return;  // the timer was cancelled

  // Forcefully close the socket to make sure that the active connection attempt has been stopped
  // as well. This also solves a potential issue where the Promise gets resolved twice.
  if (ssl_mode_ == SocketSSLMode::kNone)
    ws_stream_->close(boost::beast::websocket::close_code::none);
  else
    wss_stream_->close(boost::beast::websocket::close_code::none);

  CallOnMainThread(boost::bind(open_callback, boost::asio::error::timed_out));
}

void WebSocket::Read(ReadCallback read_callback, ErrorCallback error_callback) {
  auto callback = boost::bind(&WebSocket::OnRead, this, read_callback, error_callback,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred);
  if (ssl_mode_ == SocketSSLMode::kNone)
    ws_stream_->async_read_some(boost::asio::buffer(read_buffer_), callback);
  else
    wss_stream_->async_read_some(boost::asio::buffer(read_buffer_), callback);
}

void WebSocket::OnRead(ReadCallback read_callback,
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

void WebSocket::Write(void* data, std::size_t bytes, WriteCallback write_callback) {
  if (active_write_) {
    write_queue_.push(boost::bind(&WebSocket::Write, this, data, bytes, write_callback));
    return;
  }

  active_write_ = true;

  auto buffer = boost::asio::buffer(data, bytes);
  auto callback = boost::bind(&WebSocket::OnWrite, this, write_callback,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred);

  if (ssl_mode_ == SocketSSLMode::kNone)
    ws_stream_->async_write(buffer, callback);
  else
    wss_stream_->async_write(buffer, callback);
}

void WebSocket::OnWrite(WriteCallback write_callback,
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

void WebSocket::Close(CloseCallback close_callback) {}

}  // namespace socket
}  // namespace bindings
