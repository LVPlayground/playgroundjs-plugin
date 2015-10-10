// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/logging.h"

#include <iostream>
#include <stdio.h>

#if defined(LINUX)
#include <signal.h>
#include <unistd.h>
#endif

namespace logging {

namespace {

// The log handler that should handle output, if any.
std::unique_ptr<LogMessage::LogHandler> g_handler;

// Textual representations of the severity levels.
const char kSeverityInfo[] = "INFO";
const char kSeverityWarning[] = "WARNING";
const char kSeverityError[] = "ERROR";
const char kSeverityFatal[] = "FATAL";
const char kSeverityUnknown[] = "UNKNOWN";

const char* SeverityToString(Severity severity) {
  switch (severity) {
  case Info:
    return kSeverityInfo;
  case Warning:
    return kSeverityWarning;
  case Error:
    return kSeverityError;
  case Fatal:
    return kSeverityFatal;
  }

  return kSeverityUnknown;
}

}  // namespace

// static
void LogMessage::SetLogHandler(std::unique_ptr<LogMessage::LogHandler> handler) {
  g_handler.swap(handler);
}

LogMessage::LogMessage(const char* file, unsigned int line, Severity severity)
    : file_(file),
      line_(line),
      severity_(severity) {}

LogMessage::~LogMessage() {
  const char* severity = SeverityToString(severity_);

  std::string filename = file_;
  std::string message = stream_.str();

  // Only use the filename, not the path, for the output message.
  size_t file_offset = filename.find_last_of("\\/");
  if (file_offset != std::string::npos)
    filename.assign(filename.begin() + file_offset + 1, filename.end());

  if (g_handler)
    g_handler->Write(severity, filename.c_str(), line_, message.c_str());
  else
    fprintf(stderr, "[%s][%s:%d] %s\n", severity, filename.c_str(), line_, message.c_str());

  if (severity_ != Fatal)
    return;

#ifndef NDEBUG
#if defined(WIN32)
  __debugbreak();
#else
  raise(SIGTRAP);
#endif
#endif

  // Wait for any further input from the user to halt the console.
  system("PAUSE");

  // Exit the program, we're done.
  _exit(1);
}

}  // namespace logging
