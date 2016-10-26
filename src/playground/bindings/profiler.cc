// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/profiler.h"

#include "base/logging.h"
#include "base/time.h"

#include <fstream>

#include <include/v8.h>
#include <include/v8-profiler.h>

namespace bindings {

namespace {

// Class responsible for printing captured CPU profiles to a file. Will open a file stream
// in the constructor, and close it again in the destructor.
class ProfilePrinter {
 public:
  ProfilePrinter(const std::string& filename, v8::CpuProfile* profile)
      : stream_(filename.c_str(), std::ios_base::out | std::ios_base::trunc),
        profile_(profile) {}

  ~ProfilePrinter() { stream_.close(); }

  bool Print() {
    // TODO: Implement the actual printer.
    return false;
  }

 private:
  std::ofstream stream_;
  v8::CpuProfile* profile_;
};

}  // namespace

Profiler::Profiler(v8::Isolate* isolate)
    : cpu_profiler_(v8::CpuProfiler::New(isolate)),
      isolate_(isolate) {}

Profiler::~Profiler() {
  cpu_profiler_->Dispose();
}

void Profiler::Profile(int32_t milliseconds, const std::string& filename) {
  DCHECK(milliseconds >= 1000 && milliseconds <= 180000);

  completion_time_ = base::monotonicallyIncreasingTime() + milliseconds;
  filename_ = filename;
  active_ = true;

  LOG(INFO) << "Starting a profile for " << milliseconds << "ms (" << filename << ")";
  cpu_profiler_->StartProfiling(v8::String::Empty(isolate_), true /* record_samples */);
}

void Profiler::OnFrame(double current_time) {
  if (current_time <= completion_time_)
    return;

  active_ = false;

  v8::CpuProfile* profile = cpu_profiler_->StopProfiling(v8::String::Empty(isolate_));
  {
    int32_t samples = profile->GetSamplesCount();

    // Both these values will be in microseconds from an arbitrary base.
    int64_t start_time = profile->GetStartTime();
    int64_t end_time = profile->GetEndTime();

    int64_t time_ms = ((end_time - start_time) + 1000 - 1) / 1000;

    LOG(INFO) << "Finished the profile. Captured " << samples << " samples covering " << time_ms << "ms.";

    // Print the profile to the |filename_| using a profile printer.
    ProfilePrinter printer(filename_, profile);
    if (printer.Print())
      LOG(INFO) << "Wrote the profile to " << filename_ << ".";
    else
      LOG(INFO) << "Unable to write the profile to " << filename_ << ": an error occurred.";
  }
  profile->Delete();
}

}  // namespace bindings
