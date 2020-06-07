// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket.h"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "base/logging.h"
#include "bindings/modules/socket/socket_open_options.h"
#include "bindings/exception_handler.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {
namespace socket {

namespace {

template <typename T>
void ResolvePromise(std::shared_ptr<Promise> promise, T value) {
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

void Socket::Open(SocketOpenOptions options, std::shared_ptr<Promise> promise) {
  switch (state_) {
    case State::kConnecting:
    case State::kConnected:
    case State::kDisconnecting:
      ThrowException("Unable to open the socket: there already is an active connection.");
      return;

    case State::kDisconnected:
      break;
  }

  state_ = State::kConnecting;
  engine_->Open(std::move(options), 
                boost::bind(&Socket::OnConnect, this, boost::asio::placeholders::error, promise));
}

void Socket::OnConnect(const boost::system::error_code& ec, std::shared_ptr<Promise> promise) {
  if (ec) {
    state_ = State::kDisconnected;

    // Invoke the `error` event listeners with information about the issue. We skip this part
    // for timeouts, because those are a more common failure in the connection process.
    if (ec.value() != boost::asio::error::timed_out)
      observer_->OnError(ec.value(), ec.message());

  } else {
    state_ = State::kConnected;

    // Begin consuming information from the socket. This is an asynchronous process that will
    // continue for the socket's lifetime. `message` events will fire upon data availability.
    engine_->Read(boost::bind(&Socket::OnRead, this, boost::placeholders::_1),
                  boost::bind(&Socket::OnError, this, boost::asio::placeholders::error));
  }
  
  ResolvePromise(std::move(promise), /* success= */ !ec);
}

void Socket::Write(uint8_t* data, size_t bytes, std::shared_ptr<Promise> promise) {
  switch (state_) {
    case State::kConnecting:
    case State::kDisconnecting:
    case State::kDisconnected:
      ThrowException("Unable to write data to the socket: this requires an active connection.");
      return;

    case State::kConnected:
      break;
  }

  WriteBuffer buffer = std::make_shared<std::vector<uint8_t>>(data, data + bytes);

  // Writes the given |buffer| to the socket. The |buffer| will be passed to the callback in order
  // to keep the associated memory alive. The |promise| will be invoked once successful.
  engine_->Write(&buffer->front(), bytes,
                 boost::bind(&Socket::OnWrite, this, boost::asio::placeholders::error,
                             boost::asio::placeholders::bytes_transferred, promise, buffer));
}

void Socket::OnWrite(const boost::system::error_code& ec,
                     std::size_t bytes_transferred,
                     std::shared_ptr<Promise> promise,
                     WriteBuffer buffer) {
  ResolvePromise(std::move(promise), /* success= */ !ec);

  if (ec)
    observer_->OnError(ec.value(), ec.message());
}

void Socket::Close(std::shared_ptr<Promise> promise) {
  switch (state_) {
    case State::kDisconnected:
      ThrowException("Unable to close the socket: this requires a connection.");
      return;

    case State::kConnecting:
    case State::kConnected:
    case State::kDisconnecting:
      break;
  }

  state_ = State::kDisconnecting;
  engine_->Close(boost::bind(&Socket::OnClose, this, std::move(promise)));
}

void Socket::OnClose(std::shared_ptr<Promise> promise) {
  state_ = State::kDisconnected;
  observer_->OnClose();
}

void Socket::OnRead(std::shared_ptr<std::vector<uint8_t>> data) {
  observer_->OnMessage(&data->front(), data->size());
}

void Socket::OnError(const boost::system::error_code& ec) {
  observer_->OnError(ec.value(), ec.message());
}

}  // namespace socket
}  // namespace bindings
