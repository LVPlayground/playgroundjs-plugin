// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "performance/trace_manager.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>

#include "base/file_path.h"
#include "base/logging.h"
#include "performance/trace.h"

namespace performance {

namespace {

// Number of seconds of wall clock execution time after a warning should be issued to the console
// when a trace is active and a slow callback is observed.
const double TraceWarningThresholdSeconds = 0.5;

// Global instance of the trace manager.
std::unique_ptr<TraceManager> g_trace_manager_;

}  // namespace

// static
TraceManager* TraceManager::GetInstance() {
  if (!g_trace_manager_)
    g_trace_manager_.reset(new TraceManager);

  return g_trace_manager_.get();
}

TraceManager::TraceManager() 
    : enabled_(false) {}

void TraceManager::Capture(const Trace& trace) {
  std::lock_guard<std::mutex> scoped_lock(captured_traces_lock_);
  captured_traces_.push_back(trace);

  if (trace.type != INTERCEPTED_CALLBACK_TOTAL)
    return;  // never issue warnings for this type

  const double diff = trace.end - trace.start;
  if (diff < TraceWarningThresholdSeconds)
    return;

  LOG(WARNING) << "Event " << trace.details[0] << " took a long time: " << diff << "ms";
}

void TraceManager::Write(const base::FilePath& file, bool clear_traces) {
  std::vector<Trace> captured_traces_copy;
  {
    std::lock_guard<std::mutex> scoped_lock(captured_traces_lock_);
    std::copy(captured_traces_.begin(), captured_traces_.end(),
              std::back_inserter(captured_traces_copy));

    if (clear_traces)
      captured_traces_.clear();
  }

  std::ofstream stream;
  stream.open(file.value().c_str(), std::ios::out | std::ios::trunc);
  
  if (!stream.is_open()) {
    LOG(ERROR) << "Unable to open " << file.value() << " for writing.";
    return;
  }

  for (const Trace& trace : captured_traces_copy) {
    stream << trace.type << "|"
           << std::fixed << std::setprecision(4) << trace.start << "|"
           << std::fixed << std::setprecision(4) << trace.end << "|"
           << trace.details[0] << "|" << trace.details[1] << std::endl;
  }

  stream.close();
}

}  // namespace performance
