// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_

#include <boost/function.hpp>
#include <boost/system/system_error.hpp>

#include <cstddef>
#include <stdint.h>
#include <string>
#include <vector>

#include "bindings/modules/socket/socket_open_options.h"

namespace bindings {
namespace socket {

// Base class for all socket operations. Methods are implemented by the specific socket types, for
// example the TcpSocket class.
class BaseSocket {
 public:
  // Callback signatures that should be passed to some of the socket methods.
  using CloseCallback = boost::function<void()>;
  using ErrorCallback = boost::function<void(const boost::system::error_code& ec)>;
  using OpenCallback = boost::function<void(const boost::system::error_code& ec)>;
  using ReadCallback = boost::function<void(std::shared_ptr<std::vector<uint8_t>>)>;
  using WriteCallback = boost::function<void(const boost::system::error_code& ec, std::size_t bytes)>;

  virtual ~BaseSocket() = default;

  // Opens a connection with the given |options|. The |open_callback| will be invoked when the
  // connection attempt has finished, regardless of result.
  virtual void Open(SocketOpenOptions options, OpenCallback open_callback) = 0;

  // Starts reading data from the socket. The |error_callback| will be invoked when an error
  // occurred, otherwise the |read_callback| will be called every time more data is available.
  virtual void Read(ReadCallback read_callback, ErrorCallback error_callback) = 0;

  // Writes the given |data| to the socket, which is |bytes| in length. The data must outlive
  // this call, i.e. be destroyed no earlier than the |write_callback| being invoked.
  virtual void Write(void* data, std::size_t bytes, WriteCallback write_callback) = 0;

  // Closes the connection that's been established by this socket, if any.
  virtual void Close(CloseCallback close_callback) = 0;
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_
