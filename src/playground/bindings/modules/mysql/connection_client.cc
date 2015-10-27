// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql/connection_client.h"

#ifndef WIN32
#undef max(a, b)
#undef min(a, b)
#endif

#include <memory>

#include "base/logging.h"
#include "playground/bindings/modules/mysql/result_entry.h"

namespace mysql {

const int ConnectionRetryIntervalMs = 5000;
const int ServerPingIntervalMs = 5000;

const my_bool my_Enable = 1;

static ColumnInfo::ColumnType toColumnType(enum_field_types type) {
  switch (type) {
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_FLOAT:
  case MYSQL_TYPE_DOUBLE:
    return ColumnInfo::FloatColumnType;

  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_BIT:
  case MYSQL_TYPE_LONGLONG:
  case MYSQL_TYPE_INT24:
  case MYSQL_TYPE_YEAR:
    return ColumnInfo::IntegerColumnType;
  }

  return ColumnInfo::StringColumnType;
}

// -----------------------------------------------------------------------------
// Client-thread methods.

void ConnectionClient::doConnect() {
  mysql_init(&connection_);
  mysql_options(&connection_, MYSQL_OPT_RECONNECT, &my_Enable);

  ConnectionMessages::ConnectionAttemptResult result;
  result.id = connection_status_.information.id;

  if (mysql_real_connect(&connection_, connection_status_.information.hostname.c_str(),
                         connection_status_.information.username.c_str(), connection_status_.information.password.c_str(),
                         connection_status_.information.database.c_str(), connection_status_.information.port, nullptr, 0)) {
    result.succeeded = true;
    result.error_number = 0;
    result.error_message = "";

    // Mark the connection as being connected.
    connection_status_.is_connected = true;

  } else {
    result.succeeded = false;
    result.error_number = mysql_errno(&connection_);
    result.error_message = std::string(mysql_error(&connection_));
  }

  connection_attempt_queue_.push(result);
}

void ConnectionClient::doClose() {
  mysql_close(&connection_);
}

void ConnectionClient::doPing() {
  mysql_ping(&connection_);
}

void ConnectionClient::doQuery(unsigned int request_id, const std::string& query, ExecutionType execution_type) {
  if (mysql_real_query(&connection_, query.c_str(), query.size()) == 0) {
    MYSQL_RES* query_result = mysql_store_result(&connection_);
    std::shared_ptr<ResultEntry> result(new ResultEntry);

    // In some cases we might not want any feedback at all.
    if (execution_type == SilentExecution)
      return;

    if (query_result != 0) {
      // This was a SELECT query that returned rows.
      int field_count = static_cast<int>(mysql_num_fields(query_result));
      int row_count = static_cast<int>(mysql_num_rows(query_result));

      if (row_count > 0) {
        MYSQL_FIELD* fields = mysql_fetch_fields(query_result);
        for (int fieldId = 0; fieldId < field_count; ++fieldId)
          result->addColumn(fields[fieldId].name, toColumnType(fields[fieldId].type));

        while (MYSQL_ROW resultRow = mysql_fetch_row(query_result)) {
          unsigned long* fieldLengths = mysql_fetch_lengths(query_result);
          RowInfo* row = result->createRow();

          for (int fieldId = 0; fieldId < field_count; ++fieldId)
            row->pushField(resultRow[fieldId], fieldLengths[fieldId], toColumnType(fields[fieldId].type));
        }
      }

      mysql_free_result(query_result);

    } else {
      if (mysql_field_count(&connection_) != 0) {
        // This was an INSERT, UPDATE or DELETE query that shouldn't return rows.
      }
      else {
        // No data-retrieval errors occurred, continue with filling the result.
        result->affected_rows_ = static_cast<int>(mysql_affected_rows(&connection_));
        result->insert_id_ = static_cast<int>(mysql_insert_id(&connection_));
      }
    }

    ConnectionMessages::SucceededQueryResult success_result;
    success_result.id = request_id;
    success_result.result_entry = result;

    succeeded_query_queue_.push(success_result);
    return;
  }

  if (execution_type == SilentExecution)
    return;

  ConnectionMessages::FailedQueryResult error_result;
  error_result.id = request_id;
  error_result.error_number = mysql_errno(&connection_);
  error_result.error_message = std::string(mysql_error(&connection_));

  failed_query_queue_.push(error_result);
}

void ConnectionClient::run() {
  mysql_thread_init();

  while (!shutdown_requested()) {
    if (connection_status_.has_connection_information == false) {
      if (connection_queue_.size() > 0) {
        // We received connection information for this connection client.
        connection_queue_.pop(connection_status_.information);
        connection_status_.has_connection_information = true;
        continue;
      }

      thread_sleep(25);
      continue;
    }

    if (connection_status_.is_connected == false) {
      if (connection_status_.last_attempt == 0 || timeSpan(connection_status_.last_attempt) > ConnectionRetryIntervalMs) {
        doConnect(); // attempt to connect to the server.
        connection_status_.last_attempt = time();
        continue;
      }

      thread_sleep(25);
      continue;
    }

    if (connection_status_.last_ping == 0 || timeSpan(connection_status_.last_ping) > ServerPingIntervalMs) {
      connection_status_.last_ping = time();
      doPing();
      continue;
    }

    if (query_queue_.size() > 0) {
      ConnectionMessages::QueryInformation information;
      query_queue_.pop(information);

      // Process this query. The results will be send back separately.
      doQuery(information.id, information.query, NormalExecution);
    }

    thread_sleep(25);
  }

  // Before shutting down, execute the remaining queries with a maximum of fifty. The San
  // Andreas multiplayer server will wait for twelve seconds until loading the gamemode again
  // as it is, which gives us 0.24 seconds per query.
  unsigned int executedQueries = 0;
  while (connection_status_.is_connected && query_queue_.size() > 0 && executedQueries++ < 50) {
    ConnectionMessages::QueryInformation information;
    query_queue_.pop(information);

    // Process the query. No information will be relayed to the gamemode anymore.
    doQuery(information.id, information.query, SilentExecution);
  }

  // Always gracefully close the connection after we're done.
  if (connection_status_.has_connection_information == true && connection_status_.is_connected == true)
    doClose();

  mysql_thread_end();
}

}  // namespace mysql
