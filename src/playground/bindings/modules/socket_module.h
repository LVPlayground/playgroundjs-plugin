// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_SOCKET_MODULE_H_
#define PLAYGROUND_BINDINGS_MODULES_SOCKET_MODULE_H_

#include <include/v8.h>

namespace bindings {

// The Socket module allows JavaScript to open networking sockets with arbitrary destinations,
// for example to open (and maintain) echo or IRC connections. The class is implemented on top of
// the Boost ASIO library.
//
// When constructing a new Socket object, the protocol backing the socket has to be decided. This
// cannot change during the lifetime of the socket. Currently, only TCP sockets are implemented.
//
// Connecting to a socket can be done through the `open()` method. This takes a dictionary that,
// at the very least, must contain the `ip` and `port` fields. Optionally a `timeout` can be
// specified as well, and certain protocol-specific options are available.
//
// ** Options available for all protocols
//
//    `ip`       The IP address that the socket should be opened to.
//    `port`     The port number (1-65535) that the socket should be opened to.
//    `timeout`  Timeout, in seconds, to wait for the socket's connection to succeed.
//
// ** Options for TCP sockets
//
//    `ssl`      Whether the connection should use SSL. Must be one of the following constants:
//               "none" | "auto" | "ssl" | "tls" | "tlsv11" | "tlsv12" | "tlsv13"
//
// Once connected, data may be written by calling the `write()` method, and the connection may be
// closed by calling the `close()` method. It is valid for multiple `write()` operations to be
// active at the same time, as protocol engines are expected to deal with that.
//
// While the socket is alive, there are three events that can be called by the instance. They
// have the following names and event structures:
//
//   close     Event { }
//   error     Event { code (number), message (string) }
//   message   Event { data (ArrayBuffer) }
//
// -------------------------------------------------------------------------------------------------
//
// enum SocketSSLMode { "none" | "auto" | "ssl" | "tls" | "tlsv11" | "tlsv12" | "tlsv13" };
//
// dictionary SocketOpenOptions {
//             string         ip;
//             number         port;
//   optional  number         timeout = 30;
//   optional  SocketSSLMode  ssl = "none";
// }
//
// [Constructor(string protocol)]
// interface Socket {
//     Promise<boolean> open(SocketOpenOptions options);
//     Promise<boolean> write(ArrayBuffer data);
//     Promise<void>    close();
//
//     void             addEventListener(string event, function listener);
//     void             removeEventListener(string event, function listener);
//
//     readonly attribute string state;
// }
class SocketModule {
 public:
  SocketModule();
  ~SocketModule();

  // Called when the prototype for the global object is being constructed. 
  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_SOCKET_MODULE_H_
