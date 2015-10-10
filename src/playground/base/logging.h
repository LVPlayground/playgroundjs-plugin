// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_LOGGING_H_
#define PLAYGROUND_BASE_LOGGING_H_

#include <memory>
#include <sstream>

namespace logging {

// Severity of the item to log. Fatal errors will invoke a crash, as a critical
// condition in the program's flow could not be satisfied.
enum Severity {
  Info,
  Warning,
  Error,
  Fatal
};

// Represents a log message to be written to either the console, or a registered handler.
class LogMessage {
 public:
  // Interface allowing external components to handle log output.
  class LogHandler {
   public:
    virtual ~LogHandler() {}
    virtual void Write(const char* severity, const char* file, unsigned int line, const char* message) = 0;
  };

  // Overrides the default log handler (which is stderr) with |handler|.
  static void SetLogHandler(std::unique_ptr<LogHandler> handler);

  LogMessage(const char* file, unsigned int line, Severity severity);
  ~LogMessage();

  // Returns the stream to which the log message can be written.
  std::ostream& stream() { return stream_; }

 private:
  const char* file_;
  unsigned int line_;
  Severity severity_;

  std::ostringstream stream_;
};

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) { }
};

}  // namespace logging

#define LAZY_STREAM(stream, condition)     \
    !(condition) ? (void) 0 : logging::LogMessageVoidify() & (stream)

#define LOG_STREAM(severity, condition)    \
    LAZY_STREAM(logging::LogMessage(__FILE__, __LINE__, logging::severity).stream(), condition)

#define LOG_FATAL     LOG_STREAM(Fatal,   true)
#define LOG_ERROR     LOG_STREAM(Error,   true)
#define LOG_WARNING   LOG_STREAM(Warning, true)
#define LOG_INFO      LOG_STREAM(Info,    true)
#define LOG(severity) LOG_ ## severity

#define CHECK(condition)                \
    LOG_STREAM(Fatal, !(condition)) << "Check failed: " #condition ". "

#define ASSERT CHECK

#if defined(NDEBUG)
// TODO: Disable this post-development.
#define DCHECK(condition) CHECK(condition)
#else
#define DCHECK(condition) CHECK(condition)
#endif

#endif  // PLAYGROUND_BASE_LOGGING_H_
