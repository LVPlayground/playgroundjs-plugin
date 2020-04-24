// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/tcp_socket.h"
#include "bindings/runtime.h"

#include <boost/bind/bind.hpp>

namespace bindings {
namespace socket {

TcpSocket::TcpSocket()
    : io_context_(bindings::Runtime::FromIsolate(v8::Isolate::GetCurrent())->io_context()),
      boost_deadline_timer_(io_context_),
      boost_tcp_socket_(io_context_) {}

TcpSocket::~TcpSocket() {
  if (boost_tcp_socket_.is_open())
    boost_tcp_socket_.close();
}

bool TcpSocket::ParseOptions(const v8::Local<v8::Object>& options) {
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

  boost_tcp_socket_.async_connect(endpoint, std::move(open_callback));

  boost_deadline_timer_.expires_from_now(boost::posix_time::seconds(timeout));
  //boost_deadline_timer_.async_wait(std::move(timeout_callback));
}

void TcpSocket::Read(ReadCallback read_callback, ErrorCallback error_callback) {
  boost_tcp_socket_.async_read_some(
      boost::asio::buffer(read_buffer_),
      boost::bind(&TcpSocket::OnRead, this, read_callback, error_callback,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
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

  boost_tcp_socket_.async_read_some(
      boost::asio::buffer(read_buffer_),
      boost::bind(&TcpSocket::OnRead, this, read_callback, error_callback,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void TcpSocket::Write(void* data, size_t bytes, WriteCallback write_callback) {
  boost_tcp_socket_.async_send(boost::asio::buffer(data, bytes), std::move(write_callback));
}

void TcpSocket::Close() {
  boost_tcp_socket_.close();
}

Protocol TcpSocket::protocol() const {
  return Protocol::kTcp;
}

}  // namespace socket
}  // namespace bindings
