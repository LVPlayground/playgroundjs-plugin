// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket.h"

#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "bindings/exception_handler.h"
#include "bindings/runtime.h"

namespace bindings {
namespace socket {

namespace {

// Message to display when the connection attempt has timed out.
const char kConnectionTimeoutError[] = "the connection attempt has timed out";

template <typename T>
void ResolvePromise(std::unique_ptr<Promise> promise, T value) {
  ScopedExceptionSource source("socket module");
  {
    std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
    if (!runtime)
      return;  // the runtime is being shut down

    v8::HandleScope handle_scope(runtime->isolate());
    v8::Context::Scope context_scope(runtime->context());

    promise->Resolve(value);
  }
}

}  // namespace

Socket::Socket(Protocol protocol, SocketObserver* observer)
    : protocol_(protocol),
      state_(State::kDisconnected),
      observer_(observer),
      io_context_(bindings::Runtime::FromIsolate(v8::Isolate::GetCurrent())->io_context()),
      boost_deadline_timer_(io_context_),
      boost_socket_(io_context_) {}

Socket::~Socket() {
  if (state_ != State::kDisconnected)
    boost_socket_.close();
}

void Socket::Open(const std::string& ip, uint16_t port, int32_t timeout, std::unique_ptr<bindings::Promise> promise) {
  connection_promise_ = std::move(promise);

  boost::asio::ip::tcp::endpoint endpoint{
      boost::asio::ip::address_v4::from_string(ip),
      port };

  boost_socket_.async_connect(
      endpoint,
      boost::bind(&Socket::OnConnect, this, boost::asio::placeholders::error));

  boost_deadline_timer_.expires_from_now(boost::posix_time::seconds(timeout));
  boost_deadline_timer_.async_wait(boost::bind(&Socket::OnConnectTimeout, this));

  state_ = State::kConnecting;
}

void Socket::OnConnect(const boost::system::error_code& ec) {
  if (ec) {
    observer_->OnError(ec.value(), ec.message());

    state_ = State::kDisconnected;
    boost_socket_.close();

  } else {
    state_ = State::kConnected;
    boost_socket_.async_read_some(boost::asio::buffer(read_buffer_),
                                  boost::bind(&Socket::OnRead, this, boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
  }

  ResolvePromise(std::move(connection_promise_), /* success= */ !ec);
}

void Socket::OnConnectTimeout() {
  if (this->state_ != State::kConnecting)
    return;

  observer_->OnError(boost::asio::error::timed_out, kConnectionTimeoutError);

  state_ = State::kDisconnected;
  ResolvePromise(std::move(connection_promise_), /* success= */ false);
}

void Socket::OnRead(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec) {
    observer_->OnError(ec.value(), ec.message());

  } else {
    observer_->OnMessage(&read_buffer_.front(), bytes_transferred);

    boost_socket_.async_read_some(boost::asio::buffer(read_buffer_),
                                  boost::bind(&Socket::OnRead, this, boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
  }
}

void Socket::Write(uint8_t* data, size_t bytes, std::unique_ptr<Promise> promise) {
  std::unique_ptr<std::vector<uint8_t>> buffer =
      std::make_unique<std::vector<uint8_t>>(data, data + bytes);

  auto boost_buffer = boost::asio::buffer(*buffer);

  std::shared_ptr<WriteData> write_data =
      std::make_shared<WriteData>(std::move(promise), std::move(buffer));

  boost_socket_.async_send(boost_buffer,
                           boost::bind(&Socket::OnWrite, this, boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred, write_data));
}

void Socket::OnWrite(const boost::system::error_code& ec,
                     std::size_t bytes_transferred,
                     std::shared_ptr<WriteData> write_data) {
  if (ec)
    observer_->OnError(ec.value(), ec.message());

  ResolvePromise(std::move(write_data->promise), /* success= */ !ec);
}

void Socket::Close() {
  state_ = State::kDisconnected;
  boost_socket_.close();
}

}  // namespace socket
}  // namespace bindings
