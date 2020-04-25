// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_

#include <boost/function.hpp>
#include <boost/system/system_error.hpp>

#include <include/v8.h>

#include <stdint.h>
#include <string>

namespace bindings {
namespace socket {

// Network protocol that should be used by a created socket.
enum class Protocol {
  kTcp
};

// Base class for all socket operations. Methods are implemented by the specific socket types, for
// example the TcpSocket class.
class BaseSocket {
 public:
  // Callback signatures that should be passed to some of the socket methods.
  using ErrorCallback = boost::function<void(const boost::system::error_code& ec)>;
  using OpenCallback = boost::function<void(const boost::system::error_code& ec)>;
  using ReadCallback = boost::function<void(void* data, std::size_t size)>;
  using TimeoutCallback = boost::function<void(const boost::system::error_code& ec)>;
  using WriteCallback = boost::function<void(const boost::system::error_code& ec, std::size_t bytes)>;

  virtual ~BaseSocket() = default;

  // Parses the given |options|, which are specific to the kind of socket. State should be stared
  // locally in socket, but does not have to be reflected on the object later on.
  virtual bool ParseOptions(v8::Local<v8::Context> context, v8::Local<v8::Object> options) = 0;

  // Opens a connection to the given |ip| and |port|. If the connection has not been established
  // after |timeout| seconds, the attempt will be considered as a failed one.
  virtual void Open(const std::string& ip,
                    uint16_t port,
                    int32_t timeout,
                    OpenCallback open_callback,
                    TimeoutCallback timeout_callback) = 0;

  // Starts reading data from the socket. The |error_callback| will be invoked when an error
  // occurred, otherwise the |read_callback| will be called every time more data is available.
  virtual void Read(ReadCallback read_callback, ErrorCallback error_callback) = 0;

  // Writes the given |data| to the socket, which is |bytes| in length. The data must outlive
  // this call, i.e. be destroyed no earlier than the |write_callback| being invoked.
  virtual void Write(void* data, size_t bytes, WriteCallback write_callback) = 0;

  // Closes the connection that's been established by this socket, if any.
  virtual void Close() = 0;

  // Returns the protocol that this socket engine implements.
  virtual socket::Protocol protocol() const = 0;
};

}  // namespace socket
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_BASE_SOCKET_H_
