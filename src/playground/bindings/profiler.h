// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include <memory>
#include <stdint.h>
#include <string>

namespace v8 {
class CpuProfiler;
class Isolate;
}

namespace bindings {

// The profiler enables measuring of JavaScript execution time for a limited amount of time that
// can be expressed in a detailed flame chart. It will contain as much detailed information on
// the performance of Las Venturas Playground as we can gather.
class Profiler {
 public:
  explicit Profiler(v8::Isolate* isolate);
  ~Profiler();

  // Returns whether a CPU profile is currently being recorded.
  bool IsActive() const { return active_; }

  // Captures a CPU profile lasting |seconds| and writes the result to |filename| once completed.
  void Profile(int32_t seconds, const std::string& filename);

  // Called each server frame while a CPU profile is active.
  void OnFrame(double current_time);

 private:
  // Weak reference. Will be disposed of by the constructor.
  v8::CpuProfiler* cpu_profiler_;

  // Indicates whether a profile is currently being recorded.
  bool active_ = false;

  // Time at which the in-progress profile should stop.
  double completion_time_ = 0;
};

}  // namespace bindings
