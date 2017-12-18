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

ExceptionHandler::ExceptionHandler(Runtime* runtime, Runtime::Delegate* runtime_delegate)
    : runtime_(runtime), runtime_delegate_(runtime_delegate) {}

ExceptionHandler::~ExceptionHandler() {}

void ExceptionHandler::OnMessage(v8::Local<v8::Message> message, v8::Local<v8::Value> error, MessageSource source,
                                 v8::Local<v8::Promise> promise) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (isolate->IsExecutionTerminating())
    isolate->CancelTerminateExecution();

  if (!promise.IsEmpty()) {
    queued_messages_.emplace_back(isolate, message, error, source, promise);
    return;
  }

  runtime_delegate_->OnScriptOutput("=========================");

  std::string prefix;
  {
    // (1) Append the resource name.
    const base::FilePath& source_directory = runtime_->source_directory();
    const std::string resource_name = toString(message->GetScriptResourceName());

    if (!resource_name.find(source_directory.value()))
      prefix += resource_name.substr(source_directory.value().length() + 1);
    else
      prefix += resource_name;

    // (2) Append the line number.
    prefix += ":" + std::to_string(message->GetLineNumber());
    prefix += ": ";
  }

  runtime_delegate_->OnScriptOutput(prefix + toString(error));

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

void ExceptionHandler::RevokeQueuedMessages(v8::Local<v8::Promise> promise) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  for (auto iter = queued_messages_.begin(); iter != queued_messages_.end();) {
    const QueuedMessage& message = *iter;

    if (promise == v8::Local<v8::Promise>::New(isolate, message.promise))
      iter = queued_messages_.erase(iter);
    else
      iter++;
  }
}

void ExceptionHandler::FlushMessageQueue() {
  if (!queued_messages_.size())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  
  for (const QueuedMessage& message : queued_messages_) {
    OnMessage(v8::Local<v8::Message>::New(isolate, message.message),
              v8::Local<v8::Value>::New(isolate, message.error), message.message_source);
  }

  queued_messages_.clear();
}

void ExceptionHandler::OnFatalError(const char* location, const char* message) {
  LOG(ERROR) << message << " (" << location << ")";
}

}  // namespace bindings
