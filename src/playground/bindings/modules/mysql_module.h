// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_MODULE_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_MODULE_H_

#include <include/v8.h>

namespace bindings {

// The MySQL module provides JavaScript with the ability to asynchronously communicate with
// a MySQL database, for example to deal with player information.
//
// [Constructor(string hostname, string username, string password, string database, int port)]
// interface MySQL {
//     Promise<sequence<object>> query(string query);
//
//     void close();
//
//     readonly attribute Promise<> ready;
//
//     readonly attribute boolean connected;
//     readonly attribute int totalQueryCount;
//     readonly attribute int unresolvedQueryCount;
//
//     readonly attribute string hostname;
//     readonly attribute string username;
//     readonly attribute string password;
//     readonly attribute string database;
//     readonly attribute int port;
// }
class MySQLModule {
 public:
  MySQLModule();
  ~MySQLModule();

  // Called when the prototype for the global object is being constructed. 
  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_MODULE_H_
