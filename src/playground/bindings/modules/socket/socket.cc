// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket.h"

#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "base/logging.h"
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

Socket::Socket(std::unique_ptr<BaseSocket> engine, SocketObserver* observer)
    : engine_(std::move(engine)),
      state_(State::kDisconnected),
      observer_(observer) {}

Socket::~Socket() = default;

void Socket::Open(const std::string& ip, uint16_t port, int32_t timeout,
                  std::unique_ptr<bindings::Promise> promise) {
  connection_promise_ = std::move(promise);

  // TODO: Resolve |ip| if it's not an IP address.

  engine_->Open(ip, port, timeout, 
                boost::bind(&Socket::OnConnect, this, boost::asio::placeholders::error),
                boost::bind(&Socket::OnConnectTimeout, this, boost::asio::placeholders::error));

  state_ = State::kConnecting;
}

void Socket::OnConnect(const boost::system::error_code& ec) {
  if (ec) {
    observer_->OnError(ec.value(), ec.message());
    engine_->Close();

    state_ = State::kDisconnected;

  } else {
    state_ = State::kConnected;

    engine_->Read(boost::bind(&Socket::OnRead, this, boost::placeholders::_1, boost::asio::placeholders::bytes_transferred),
                  boost::bind(&Socket::OnError, this, boost::asio::placeholders::error));
  }

  CHECK(connection_promise_);
  ResolvePromise(std::move(connection_promise_), /* success= */ !ec);
}

void Socket::OnConnectTimeout(const boost::system::error_code& ec) {
  if (ec.value() == boost::asio::error::operation_aborted)
    return;

  if (this->state_ != State::kConnecting)
    return;

  observer_->OnError(boost::asio::error::timed_out, kConnectionTimeoutError);

  state_ = State::kDisconnected;

  CHECK(connection_promise_);
  ResolvePromise(std::move(connection_promise_), /* success= */ false);
}

void Socket::OnRead(void* data, std::size_t bytes) {
  observer_->OnMessage(data, bytes);
}

void Socket::OnError(const boost::system::error_code& ec) {
  observer_->OnError(ec.value(), ec.message());
}

void Socket::Write(uint8_t* data, size_t bytes, std::unique_ptr<Promise> promise) {
  std::unique_ptr<std::vector<uint8_t>> buffer =
      std::make_unique<std::vector<uint8_t>>(data, data + bytes);

  std::shared_ptr<WriteData> write_data =
      std::make_shared<WriteData>(std::move(promise), std::move(buffer));

  engine_->Write(&write_data->buffer->front(), write_data->buffer->size(),
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
  engine_->Close();
  state_ = State::kDisconnected;

  observer_->OnClose();
}

}  // namespace socket
}  // namespace bindings
