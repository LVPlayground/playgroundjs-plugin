// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql/query_result.h"

#include "base/memory.h"

#include <stdio.h>
#include <stdint.h>

#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>

namespace mysql {

int64_t g_queryResultInstanceId = 0;

QueryResult::QueryResult()
    : affected_rows_(kInvalidValue),
      insert_id_(kInvalidValue),
      instance_id_(++g_queryResultInstanceId),
      result_(nullptr) {
  LOG(ALLOC) << "QueryResult " << instance_id_;
}

QueryResult::~QueryResult() {
  LOG(ALLOC) << "~QueryResult " << instance_id_;

  if (result_) {
    mysql_free_result(result_);
    result_ = nullptr;
  }
}

}  // namespace mysql
