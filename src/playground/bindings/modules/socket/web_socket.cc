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
      resolver_(background_io_context_) {}

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

  LOG(INFO) << __FUNCTION__;
}

void WebSocket::Read(ReadCallback read_callback, ErrorCallback error_callback) {}

void WebSocket::Write(void* data, std::size_t bytes, WriteCallback write_callback) {}

void WebSocket::Close(CloseCallback close_callback) {}

}  // namespace socket
}  // namespace bindings
