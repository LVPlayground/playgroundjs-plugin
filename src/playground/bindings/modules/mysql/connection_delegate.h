// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_DELEGATE_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_DELEGATE_H_

#include <memory>
#include <string>

namespace mysql {

class QueryResult;

// Delegate that will be informed of results of the ConnectionHost class.
class ConnectionDelegate {
 public:
  // Called when a connection attempt has finished. |succeeded| indicates whether the attempt was
  // successful. If it wasn't, |error_number| and |error_message| will indicate the reason.
  virtual void DidConnect(unsigned int request_id, bool succeeded, int error_number, const std::string& error_message) = 0;

  // Called when a query has been executed successfully. The |result| will contain a result entry
  // when the query returns information, e.g. a SELECT or an INSERT/UPDATE query.
  virtual void DidQuery(unsigned int request_id, std::shared_ptr<QueryResult> result) = 0;
  
  // Called when a query has failed to execute. The |error_number| and |error_message| contain the
  // cause of the failure.
  virtual void DidQueryFail(unsigned int request_id, int error_number, const std::string& error_message) = 0;

  virtual ~ConnectionDelegate() {}
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_CONNECTION_DELEGATE_H_
