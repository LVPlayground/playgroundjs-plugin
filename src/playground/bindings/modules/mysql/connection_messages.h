// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_MESSAGES_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_MESSAGES_H_

#include <memory>
#include <string>

namespace mysql {

class QueryResult;

class ConnectionMessages {
 public:
  // Definitions for the messages we can pass between the host and client threads.
  struct ConnectionInformation {
    ConnectionInformation() : id(0), port(0) {}

    unsigned int id;
    std::string hostname, username, password, database;
    unsigned int port;
  };

  struct QueryInformation {
    QueryInformation() : id(0) {}

    unsigned int id;
    std::string query;
  };

  struct ConnectionAttemptResult {
    ConnectionAttemptResult() : id(0), succeeded(false), error_number(0) {}

    unsigned int id;
    bool succeeded;
    int error_number;
    std::string error_message;
  };

  struct FailedQueryResult {
    FailedQueryResult() : id(0), error_number(0) {}

    unsigned int id;
    int error_number;
    std::string error_message;
  };

  struct SucceededQueryResult {
    SucceededQueryResult() : id(0) {}

    unsigned int id;
    std::shared_ptr<QueryResult> result;
  };
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_MESSAGES_H_
