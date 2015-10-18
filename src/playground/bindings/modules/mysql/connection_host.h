// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_HOST_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_HOST_H_

#include <memory>
#include <string>

namespace mysql {

class ConnectionClient;
class ConnectionDelegate;

// Interface through which a MySQL connection can be established and queries can be executed. This
// class will asynchronously execute queries on a separate thread.
class ConnectionHost {
 public:
  explicit ConnectionHost(ConnectionDelegate* connection_delegate);
  ~ConnectionHost();

  // Connects to a MySQL server with the given information. Returns a request id that will be
  // included in a call to DidConnect on the delegate when completed.
  unsigned int Connect(const std::string& hostname,
                       const std::string& username,
                       const std::string& password,
                       const std::string& database,
                       unsigned int port);

  // Executes |query| on the connection when established. Returns a request id that will be
  // included in a call to DidQuery on the delegate when completed.
  unsigned int Query(const std::string& query);

  // Processes any queries that have finished on the connection thread, but have not yet
  // been announced to the |connection_delegate_|.
  void ProcessUpdates();

  // Closes the connection with the MySQL server if it has been established. The thread will
  // be closed immediately, so no feedback will be received from this operation.
  void Close();

 private:
  ConnectionDelegate* connection_delegate_;

  std::unique_ptr<ConnectionClient> client_;

  unsigned int connection_request_id_ = 0;
  unsigned int query_request_id_ = 0;
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_HOST_H_
