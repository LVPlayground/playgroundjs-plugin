// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/logging.h"
#include "bindings/modules/socket/tcp_socket.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

#include <boost/bind/bind.hpp>

namespace bindings {
namespace socket {

TcpSocket::TcpSocket()
    : security_(Security::kNone),
      io_context_(bindings::Runtime::FromIsolate(v8::Isolate::GetCurrent())->io_context()),
      boost_deadline_timer_(io_context_),
      boost_tcp_socket_(io_context_),
      active_write_(false) {}

TcpSocket::~TcpSocket() {
  if (boost_tcp_socket_.is_open())
    boost_tcp_socket_.close();
}

bool TcpSocket::ParseOptions(v8::Local<v8::Context> context, v8::Local<v8::Object> options) {
  v8::MaybeLocal<v8::Value> maybe_security_value = options->Get(context, v8String("ssl"));
  v8::Local<v8::Value> security_value;

  if (!maybe_security_value.IsEmpty() && maybe_security_value.ToLocal(&security_value)) {
    if (!security_value->IsString()) {
      ThrowException("unable to construct Socket: expected a string for the `ssl` option.");
      return false;
    }

    std::string security = toString(security_value);
    if (security == "none") {
      security_ = Security::kNone;
    } else if (security == "auto") {
      security_ = Security::kAuto;
    } else if (security == "sslv3") {
      security_ = Security::kSSLv3;
    } else if (security == "tls") {
      security_ = Security::kTLS;
    } else if (security == "tls11") {
      security_ = Security::kTLSv11;
    } else if (security == "tls12") {
      security_ = Security::kTLSv12;
    } else if (security == "tls13") {
      security_ = Security::kTLSv13;
    } else {
      ThrowException("unable to construct Socket: invalid value given for the `ssl` option.");
      return false;
    }
  }

  return true;
}

void TcpSocket::Open(const std::string& ip,
                     uint16_t port,
                     int32_t timeout,
                     OpenCallback open_callback,
                     TimeoutCallback timeout_callback) {
  boost::asio::ip::tcp::endpoint endpoint{
      boost::asio::ip::address_v4::from_string(ip),
      port };

  if (security_ != Security::kNone) {
    auto method = boost::asio::ssl::context::tls;
    switch (security_) {
      case Security::kAuto:
        method = boost::asio::ssl::context::sslv23;
        break;
      case Security::kSSLv3:
        method = boost::asio::ssl::context::sslv3;
        break;
      case Security::kTLS:
        method = boost::asio::ssl::context::tls;
        break;
      case Security::kTLSv11:
        method = boost::asio::ssl::context::tlsv11;
        break;
      case Security::kTLSv12:
        method = boost::asio::ssl::context::tlsv12;
        break;
      case Security::kTLSv13:
        method = boost::asio::ssl::context::tlsv13;
        break;
    }

    boost_ssl_context_ = std::make_unique<boost::asio::ssl::context>(method);
    boost_ssl_context_->set_verify_mode(boost::asio::ssl::verify_none);

    boost_ssl_socket_ = std::make_unique<SecureSocketType>(io_context_, *boost_ssl_context_);
    boost_ssl_socket_->next_layer().async_connect(
        endpoint, boost::bind(&TcpSocket::OnConnected, this, boost::asio::placeholders::error,
                              open_callback));
  } else {
    boost_tcp_socket_.async_connect(
        endpoint, boost::bind(&TcpSocket::OnConnected, this, boost::asio::placeholders::error,
                              open_callback));
  }
  
  boost_deadline_timer_.expires_from_now(boost::posix_time::seconds(timeout));
  boost_deadline_timer_.async_wait(std::move(timeout_callback));
}

void TcpSocket::OnConnected(const boost::system::error_code& ec,
                            OpenCallback open_callback) {
  if (ec || security_ == Security::kNone) {
    boost_deadline_timer_.cancel();
    open_callback(ec);
    return;
  }

  boost_ssl_socket_->async_handshake(
      boost::asio::ssl::stream_base::client,
      boost::bind(&TcpSocket::OnHandshakeCompleted, this, boost::asio::placeholders::error,
                  open_callback));
}

void TcpSocket::OnHandshakeCompleted(const boost::system::error_code& ec,
                                     OpenCallback open_callback) {
  boost_deadline_timer_.cancel();
  open_callback(ec);
}

void TcpSocket::Read(ReadCallback read_callback, ErrorCallback error_callback) {
  auto callback = boost::bind(&TcpSocket::OnRead, this, std::move(read_callback),
                              std::move(error_callback), boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred);

  if (security_ == Security::kNone)
    boost_tcp_socket_.async_read_some(boost::asio::buffer(read_buffer_), callback);
  else
    boost_ssl_socket_->async_read_some(boost::asio::buffer(read_buffer_), callback);
}

void TcpSocket::OnRead(ReadCallback read_callback,
                       ErrorCallback error_callback,
                       const boost::system::error_code& ec,
                       std::size_t bytes_transferred) {
  if (ec) {
    error_callback(ec);
    return;
  }

  read_callback(&read_buffer_.front(), bytes_transferred);
  Read(read_callback, error_callback);
}

void TcpSocket::Write(void* data, size_t bytes, WriteCallback write_callback) {
  if (active_write_) {
    write_buffer_.push(boost::bind(&TcpSocket::Write, this, data, bytes, write_callback));
    return;
  }

  active_write_ = true;

  auto buffer = boost::asio::buffer(data, bytes);
  auto callback = boost::bind(&TcpSocket::OnWrite, this, write_callback,
                              boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred);

  if (security_ == Security::kNone)
    boost_tcp_socket_.async_send(buffer, callback);
  else
    boost_ssl_socket_->async_write_some(buffer, callback);
}

void TcpSocket::OnWrite(WriteCallback write_callback,
                        const boost::system::error_code& ec,
                        std::size_t bytes_transferred) {
  write_callback(ec, bytes_transferred);

  active_write_ = false;

  if (!write_buffer_.empty()) {
    auto callback = write_buffer_.front();
    write_buffer_.pop();

    callback();
  }
}

void TcpSocket::Close() {
  if (security_ == Security::kNone)
    boost_tcp_socket_.close();
  else
    boost_ssl_socket_->lowest_layer().close();
}

Protocol TcpSocket::protocol() const {
  return Protocol::kTcp;
}

}  // namespace socket
}  // namespace bindings
