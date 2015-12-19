// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PERFORMANCE_TRACE_H_
#define PLAYGROUND_PERFORMANCE_TRACE_H_

#include <string>

namespace performance {

// Supported types of traces. Do not remove or modify existing values, and only add new values to
// the bottom of this enumeration (visualization tools will rely on the values).
enum TraceType {
  LOAD_JAVASCRIPT_TRACE = 0,                // time taken for loading a JavaScript file
  INTERCEPTED_CALLBACK_TOTAL = 1,           // total time for handling an intercepted Pawn callback
  INTERCEPTED_CALLBACK_EVENT_HANDLER = 2,   // time for handling an individual event handler for a callback
  TIMER_EXECUTION_TOTAL = 3,                // total time taken for executing pending timers
  // TODO: Individual timers.
  MYSQL_QUERY_START = 5,                    // time taken for starting a MySQL query
  MYSQL_QUERY_RESOLVE = 6,                  // time taken for resolving a successful MySQL query
  MYSQL_QUERY_REJECT = 7,                   // time taken for rejecting a failed MySQL query
  PAWN_NATIVE_FUNCTION_CALL = 8,            // time taken to call a native Pawn function
};

// Structure containing the information for a captured trace.
struct Trace {
  TraceType type;
  std::string details[2];
  double start, end;
};

}  // namespace performance

#endif  // PLAYGROUND_PERFORMANCE_TRACE_H_
