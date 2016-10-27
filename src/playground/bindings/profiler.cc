// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/profiler.h"

#include "base/logging.h"
#include "base/time.h"

#include <cstring>
#include <fstream>

#include <include/v8.h>
#include <include/v8-profiler.h>

namespace bindings {

namespace {

// Maximum depth of the call stack before it will be aborted.
const int32_t kMaxDepth = 32;

// Header and footer that will be written to the profile.
const char kProfileFooter[] = "]}";
const char kProfileHeader[] = R"STR(
{
  "displayTimeUnit": "ns",
  "otherData": {
    "version": "PlaygroundJS"
  },
  "samples": [],
  "traceEvents": [
)STR";

// Class responsible for printing captured CPU profiles to a file. Will open a file stream
// in the constructor, and close it again in the destructor.
class ProfilePrinter {
 public:
  ProfilePrinter(v8::CpuProfile* profile, const std::string& filename, int32_t interval_us)
      : stream_(filename.c_str(), std::ios_base::out | std::ios_base::trunc),
        profile_(profile),
        interval_us_(interval_us) {}

  ~ProfilePrinter() { stream_.close(); }

  bool Print() {
    if (!stream_.is_open())
      return false;  // unable to open the |filename_|

    if (!profile_->GetTopDownRoot())
      return false;  // we require at least the root node

    stream_ << kProfileHeader;

    begin_time_ = profile_->GetStartTime();
    escape_buffer_.reserve(1024);

    // Recursively print the samples to the opened |stream_|.
    ProcessSample(profile_->GetTopDownRoot());

    stream_ << kProfileFooter;
    return true;
  }

  void ProcessSample(const v8::CpuProfileNode* sample) {
    const char* resource_name = sample->GetScriptResourceNameStr();
    if (!resource_name || !resource_name[0])
      resource_name = "(server)";

    const char* function_name = sample->GetFunctionNameStr();
    if (!function_name || !function_name[0])
      function_name = "(anonymous function)";

    WriteBeginSample(sample);

    hit_counter_ += sample->GetHitCount() + 1;

    ++depth_;

    if (depth_ <= kMaxDepth) {
      for (int32_t index = 0; index < sample->GetChildrenCount(); ++index)
        ProcessSample(sample->GetChild(index));
    }

    --depth_;

    WriteEndSample(sample);
  }

  void WriteSampleHeader(const v8::CpuProfileNode* sample) {
    if (has_previous_sample_)
      stream_ << ",";

    stream_ << "{\"name\":\"" << EscapeString(GetFunctionNameForSample(sample)) << "\","
            << "\"cat\":\"js\",\"pid\":1,\"tid\":1,\"ts\":" << (hit_counter_ * interval_us_) << ",";

    if (sample->GetDeoptInfos().size())
      stream_ << "\"cname\":\"terrible\",";

    has_previous_sample_ = true;
  }

  void WriteBeginSample(const v8::CpuProfileNode* sample) {
    WriteSampleHeader(sample);

    const char* resource_name = sample->GetScriptResourceNameStr();
    if (!resource_name || !resource_name[0])
      resource_name = "(server)";

    const char* bailout_reason = sample->GetBailoutReason();

    stream_ << "\"ph\":\"B\",\"args\":{\"Filename\":\"" << EscapeString(resource_name) << ":";
    stream_ << sample->GetLineNumber() << "\",\"Hit count\":" << sample->GetHitCount();

    if (bailout_reason && bailout_reason[0] && strcmp(bailout_reason, "no reason"))
      stream_ << ",\"Bailout reason\":\"" << EscapeString(bailout_reason) << "\"";

    const uint32_t hit_line_count = sample->GetHitLineCount();
    if (hit_line_count) {
      line_tick_buffer_.reserve(hit_line_count);

      // This should always succeed, since we size the buffer just above here.
      CHECK(sample->GetLineTicks(&line_tick_buffer_[0], line_tick_buffer_.capacity()));

      stream_ << ",\"Line ticks\":{";

      bool first = true;
      for (uint32_t index = 0; index < hit_line_count; ++index) {
        if (!first)
          stream_ << ",";

        stream_ << "\"" << line_tick_buffer_[index].line << "\":" << line_tick_buffer_[index].hit_count;
        first = false;
      }

      stream_ << "}";
    }

    if (sample->GetDeoptInfos().size()) {
      stream_ << ",\"Deopts\":[";

      bool first = true;
      for (const auto& deopt : sample->GetDeoptInfos()) {
        if (first)
          stream_ << "\"" << EscapeString(deopt.deopt_reason) << "\"";
        else
          stream_ << ",\"" << EscapeString(deopt.deopt_reason) << "\"";

        first = false;
      }

      stream_ << "]";
    }

    stream_ << "}}";
  }

  void WriteEndSample(const v8::CpuProfileNode* sample) {
    WriteSampleHeader(sample);
    stream_ << "\"ph\":\"E\"}";
  }

  const char* GetFunctionNameForSample(const v8::CpuProfileNode* sample) {
    const char* function_name = sample->GetFunctionNameStr();
    if (!function_name || !function_name[0])
      return "(anonymous function)";

    return function_name;
  }

  bool NeedsEscape(const char* input) {
    while (*input) {
      if (*input == '\\' || *input == '"')
        return true;

      ++input;
    }

    return false;
  }

  const char* EscapeString(const char* input) {
    if (!NeedsEscape(input))
      return input;

    escape_buffer_.clear();
    while (*input) {
      switch (*input) {
      case '\\':
      case '"':
        escape_buffer_ += '\\';
      }

      escape_buffer_ += *input;

      ++input;
    }

    return escape_buffer_.c_str();
  }

 private:
  std::ofstream stream_;
  v8::CpuProfile* profile_;

  bool has_previous_sample_ = false;

  int32_t depth_ = 0;

  int64_t begin_time_;
  int32_t hit_counter_;
  int32_t interval_us_;

  std::string escape_buffer_;

  std::vector<v8::CpuProfileNode::LineTick> line_tick_buffer_;
};

}  // namespace

Profiler::Profiler(v8::Isolate* isolate)
    : cpu_profiler_(v8::CpuProfiler::New(isolate)),
      isolate_(isolate) {}

Profiler::~Profiler() {
  cpu_profiler_->Dispose();
}

void Profiler::Profile(int32_t milliseconds, const std::string& filename) {
  DCHECK(milliseconds >= 100 && milliseconds <= 180000);
  DCHECK(!active_);

  completion_time_ = base::monotonicallyIncreasingTime() + milliseconds;
  filename_ = filename;
  active_ = true;

  LOG(INFO) << "Starting a profile for " << milliseconds << "ms (" << filename << ")";

  cpu_profiler_->SetSamplingInterval(kSamplingIntervalUs);
  cpu_profiler_->StartProfiling(v8::String::Empty(isolate_), false /* record_samples */);
}

void Profiler::OnFrame(double current_time) {
  if (current_time <= completion_time_)
    return;

  active_ = false;

  v8::CpuProfile* profile = cpu_profiler_->StopProfiling(v8::String::Empty(isolate_));
  {
    int32_t samples = profile->GetSamplesCount();

    // Both these values will be in microseconds from an arbitrary base.
    int64_t time_ms = ((profile->GetEndTime() - profile->GetStartTime()) + 1000 - 1) / 1000;

    LOG(INFO) << "Finished the profile. Captured " << time_ms << "ms worth of information.";

    double start = base::monotonicallyIncreasingTime();

    // Print the profile to the |filename_| using a profile printer.
    ProfilePrinter printer(profile, filename_, kSamplingIntervalUs);
    const bool result = printer.Print();

    double duration = base::monotonicallyIncreasingTime() - start;
    
    if (result) {
      LOG(INFO) << "Wrote the profile to " << filename_ << " in " << duration << "ms.";
    } else {
      LOG(INFO) << "Unable to write the profile to " << filename_ << ": an error occurred after "
                << duration << "ms.";
    }
  }
  profile->Delete();
}

}  // namespace bindings
