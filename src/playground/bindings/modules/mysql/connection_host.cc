// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql/connection_host.h"

#include "playground/base/logging.h"
#include "playground/bindings/modules/mysql/connection_client.h"
#include "playground/bindings/modules/mysql/connection_delegate.h"

namespace mysql {

ConnectionHost::ConnectionHost(ConnectionDelegate* connection_delegate)
    : connection_delegate_(connection_delegate),
      client_(new ConnectionClient()) {
  DCHECK(connection_delegate);

  client_->StartThread();
}

ConnectionHost::~ConnectionHost() {
  client_->StopThread();
}

unsigned int ConnectionHost::Connect(const std::string& hostname,
                                     const std::string& username,
                                     const std::string& password,
                                     const std::string& database,
                                     unsigned int port) {
  ConnectionMessages::ConnectionInformation request;
  request.id = connection_request_id_;
  request.hostname = hostname;
  request.username = username;
  request.password = password;
  request.database = database;
  request.port = port;

  client_->connection_queue_.push(request);

  return connection_request_id_++;
}

unsigned int ConnectionHost::Query(const std::string& query) {
  ConnectionMessages::QueryInformation request;
  request.id = query_request_id_;
  request.query = query;

  client_->query_queue_.push(request);

  return query_request_id_++;
}

void ConnectionHost::ProcessUpdates() {
  if (client_->connection_attempt_queue_.size() > 0) {
    ConnectionMessages::ConnectionAttemptResult result;
    client_->connection_attempt_queue_.pop(&result);

    connection_delegate_->DidConnect(result.id, result.succeeded, result.error_number, result.error_message);
  }

  if (client_->succeeded_query_queue_.size() > 0) {
    ConnectionMessages::SucceededQueryResult result;
    client_->succeeded_query_queue_.pop(&result);

    connection_delegate_->DidQuery(result.id, result.result);
  }

  if (client_->failed_query_queue_.size() > 0) {
    ConnectionMessages::FailedQueryResult result;
    client_->failed_query_queue_.pop(&result);

    connection_delegate_->DidQueryFail(result.id, result.error_number, result.error_message);
  }
}

void ConnectionHost::Close() {
  client_->StopThread();
}

}  // namespace mysql
