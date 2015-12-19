// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PERFORMANCE_SCOPED_TRACE_H_
#define PLAYGROUND_PERFORMANCE_SCOPED_TRACE_H_

#include <string>

#include "base/macros.h"
#include "performance/trace.h"

namespace v8 {
class ScriptOrigin;
}

namespace performance {

// Scoped traces will automatically measure the time taken by starting the trace in the constructor,
// whilst marking it as finished (and registering it) in the destructor. Several constructors are
// available, accepting different data types, to minimize overhead when tracing is not enabled.
//
// Traces may be captured from any thread, as the TraceManager is thread safe when doing mutations
// to its captured state (i.e. capturing traces, or writing he state to a file).
class ScopedTrace {
 public:
  ScopedTrace(TraceType type, const std::string& details);
  ScopedTrace(TraceType type, const std::string& details, const v8::ScriptOrigin& origin, int line_number);
  ~ScopedTrace();

 private:
  bool capturing_;
  Trace trace_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTrace);
};

}  // namespace performance

#endif  // PLAYGROUND_PERFORMANCE_SCOPED_TRACE_H_
