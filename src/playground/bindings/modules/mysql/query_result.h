// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_QUERY_RESULT_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_QUERY_RESULT_H_

#include <stdint.h>

typedef struct st_mysql_res MYSQL_RES;

namespace mysql {

// Stores information associated with a finished query. Owns the stored result when available.
class QueryResult {
 public:
  QueryResult();
  ~QueryResult();

  // Gets or sets the number of affected rows by this query.
  int64_t affected_rows() const { return affected_rows_; }
  bool has_affected_rows() const { return affected_rows_ != kInvalidValue; }
  void set_affected_rows(int64_t affected_rows) { affected_rows_ = affected_rows; }

  // Gets or sets the id of the newly inserted row by this query.
  int64_t insert_id() const { return insert_id_; }
  bool has_insert_id() const { return insert_id_ != kInvalidValue; }
  void set_insert_id(int64_t insert_id) { insert_id_ = insert_id; }

  // Gets or sets the result associated with the query. Ownership will stay with this class.
  MYSQL_RES* result() const { return result_; }
  bool has_result() const { return result_ != nullptr; }
  void set_result(MYSQL_RES* result) { result_ = result; }

 private:
  // Integer value used to represent an invalid value for affected row count, and the inserted id.
  static const int64_t kInvalidValue = -1;

  int64_t affected_rows_;
  int64_t insert_id_;

  int64_t instance_id_;

  MYSQL_RES* result_;
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_QUERY_RESULT_H_
