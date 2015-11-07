// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql/query_result.h"

#include <stdio.h>

#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>

namespace mysql {

QueryResult::QueryResult()
    : affected_rows_(kInvalidValue),
      insert_id_(kInvalidValue),
      result_(nullptr) {}

QueryResult::~QueryResult() {
  if (result_) {
    mysql_free_result(result_);
    result_ = nullptr;
  }
}

}  // namespace mysql
