// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "performance/scoped_trace.h"

#include "base/time.h"
#include "performance/trace_manager.h"

namespace performance {

ScopedTrace::ScopedTrace(TraceType type, const std::string& details)
    : capturing_(TraceManager::GetInstance()->enabled()) {
  if (!capturing_)
    return;

  trace_.type = type;
  trace_.details = details;
  trace_.start = base::monotonicallyIncreasingTime();
}

ScopedTrace::~ScopedTrace() {
  if (!capturing_)
    return;

  trace_.end = base::monotonicallyIncreasingTime();

  TraceManager::GetInstance()->Capture(trace_);
}

}  // namespace performance
