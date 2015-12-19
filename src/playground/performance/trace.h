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
  // Trace capturing the time taken for loading a JavaScript file.
  LOAD_JAVASCRIPT_TRACE
};

// Structure containing the information for a captured trace.
struct Trace {
  TraceType type;
  std::string details;
  double start, end;
};

}  // namespace performance

#endif  // PLAYGROUND_PERFORMANCE_TRACE_H_
