// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PERFORMANCE_TRACE_MANAGER_H_
#define PLAYGROUND_PERFORMANCE_TRACE_MANAGER_H_

#include <mutex>
#include <vector>

namespace base {
class FilePath;
}

namespace performance {

struct Trace;

// The trace manager is responsible for tracking whether traces should be captured, as well as
// storing captured traces and providing the ability to write them to a file.
//
// Capturing traces is disabled by default, as it does add some overhead to execution. It can be
// enabled from either C++ or from JavaScript when capturing metrics is desired.
class TraceManager {
 public:
  // Returns the global instance of the trace manager.
  static TraceManager* GetInstance();

  // Captures |trace| and adds it to the internal buffer that keeps track of the traces. Traces may
  // be captured from any thread, internally a mutex will be held when mutating the captured state.
  void Capture(const Trace& trace);

  // Writes the captured traces to |file|. Existing content will be overridden. If |clear_traces| is
  // set to true, the currently captured traces will be removed from memory.
  //
  // The format of written trace entries is as follows (one trace per line):
  //
  // TYPE:int|START:double|END:double|DETAILS:string\n
  void Write(const base::FilePath& file, bool clear_traces);

  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) { enabled_ = enabled; }

 private:
  TraceManager();

  bool enabled_;
  std::mutex captured_traces_lock_;
  std::vector<Trace> captured_traces_;
};

}  // namespace performance

#endif  // PLAYGROUND_PERFORMANCE_TRACE_MANAGER_H_
