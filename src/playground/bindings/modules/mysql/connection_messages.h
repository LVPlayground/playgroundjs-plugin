// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_MESSAGES_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_MESSAGES_H_

#include <memory>
#include <string>

#include "playground/bindings/modules/mysql/result_entry.h"

namespace mysql {

class ConnectionMessages {
 protected:
  // Definitions for the messages we can pass between the host and client threads.
  struct ConnectionInformation {
    unsigned int id;
    std::string hostname, username, password, database;
    unsigned int port;
  };

  struct QueryInformation {
    unsigned int id;
    std::string query;
  };

  struct ConnectionAttemptResult {
    unsigned int id;
    bool succeeded;
    int error_number;
    std::string error_message;
  };

  struct FailedQueryResult {
    unsigned int id;
    int error_number;
    std::string error_message;
  };

  struct SucceededQueryResult {
    unsigned int id;
    std::shared_ptr<ResultEntry> result_entry;
  };
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_MESSAGES_H_
