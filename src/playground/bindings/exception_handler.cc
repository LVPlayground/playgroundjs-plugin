// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/exception_handler.h"

#include "base/logging.h"
#include "bindings/utilities.h"

#include <include/v8.h>

#include <stack>
#include <sstream>
#include <string>

namespace bindings {

namespace {

// Representation of anonymous functions in exception output.
const char kAnonymousFunction[] = "`anonymous function`";

// Stack of exception sources. Entries can only be added or removed by using ScopedExceptionSource
// instances when invoking functionality on the v8 runtime.
std::stack<std::string> g_exception_sources_;

}  // namespace

ScopedExceptionSource::ScopedExceptionSource(const std::string& source) {
  g_exception_sources_.push(source);
}

ScopedExceptionSource::~ScopedExceptionSource() {
  g_exception_sources_.pop();
}

ExceptionHandler::ExceptionHandler(Runtime::Delegate* runtime_delegate)
    : runtime_delegate_(runtime_delegate) {}

ExceptionHandler::~ExceptionHandler() {}

void ExceptionHandler::OnMessage(v8::Local<v8::Message> message, v8::Local<v8::Value> error, MessageSource source) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (isolate->IsExecutionTerminating())
    isolate->CancelTerminateExecution();

  runtime_delegate_->OnScriptOutput("=========================");

  const char* message_prefix = source == MessageSource::kRejectedPromise ? "Uncaught promise rejection: "
                                                                         : "";

  runtime_delegate_->OnScriptOutput(message_prefix + toString(error));

  v8::Local<v8::StackTrace> stack_trace = message->GetStackTrace();
  for (int frame = 0; frame < stack_trace->GetFrameCount(); ++frame) {
    v8::Local<v8::StackFrame> stack_frame = stack_trace->GetFrame(frame);
    v8::Local<v8::String> function_name = stack_frame->GetFunctionName();

    const std::string function =
        function_name->Length() ? toString(function_name)
                                : kAnonymousFunction;

    const std::string file_name = toString(stack_frame->GetScriptName());
    const int line_number = stack_frame->GetLineNumber();

    std::stringstream stream;
    stream << "    in " << function << " (" << file_name << ":" << line_number << ")";

    runtime_delegate_->OnScriptOutput(stream.str());
  }

  if (g_exception_sources_.size())
    runtime_delegate_->OnScriptOutput("    from " + g_exception_sources_.top());

  runtime_delegate_->OnScriptOutput("=========================");
}

void ExceptionHandler::OnFatalError(const char* location, const char* message) {
  LOG(ERROR) << message << " (" << location << ")";
}

}  // namespace bindings
