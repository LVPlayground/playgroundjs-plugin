// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/profiler.h"

#include "base/time.h"

#include <include/v8.h>
#include <include/v8-profiler.h>

namespace bindings {

Profiler::Profiler(v8::Isolate* isolate)
    : cpu_profiler_(v8::CpuProfiler::New(isolate)) {}

Profiler::~Profiler() {
  cpu_profiler_->Dispose();
}

void Profiler::Profile(int32_t seconds, const std::string& filename) {
  completion_time_ = base::monotonicallyIncreasingTime() + seconds * 1000;
  active_ = true;

  // TODO: Start the CPU profile.
}

void Profiler::OnFrame(double current_time) {
  if (current_time <= completion_time_)
    return;

  active_ = false;

  // TODO: Finalize the CPU profile.
}

}  // namespace bindings
