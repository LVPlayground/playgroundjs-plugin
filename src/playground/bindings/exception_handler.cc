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

// Stack of sources that thrown exceptions should be attributed to.
std::stack<std::pair<base::FilePath, int>> g_attribution_stack_;

}  // namespace

ScopedExceptionAttribution::ScopedExceptionAttribution(
    const base::FilePath& path, int line) {
  g_attribution_stack_.emplace(path, line);
}

ScopedExceptionAttribution::~ScopedExceptionAttribution() {
  g_attribution_stack_.pop();
}

// static
bool ScopedExceptionAttribution::HasAttribution() {
  return g_attribution_stack_.size() > 0;
}

void RegisterError(v8::Local<v8::Value> error) {
  if (!ScopedExceptionAttribution::HasAttribution())
    return;

  const auto& pair = g_attribution_stack_.top();

  Runtime::FromIsolate(v8::Isolate::GetCurrent())->GetExceptionHandler()
      ->RegisterAttributedError(error, pair.first, pair.second);
}

ScopedExceptionSource::ScopedExceptionSource(const std::string& source) {
  g_exception_sources_.push(source);
}

ScopedExceptionSource::~ScopedExceptionSource() {
  g_exception_sources_.pop();
}

ExceptionHandler::ExceptionHandler(Runtime* runtime, Runtime::Delegate* runtime_delegate)
    : runtime_(runtime), runtime_delegate_(runtime_delegate) {}

ExceptionHandler::~ExceptionHandler() {}

void ExceptionHandler::RegisterAttributedError(v8::Local<v8::Value> error, const base::FilePath& path, int line) {
  if (registered_attribution_.size() > 100)
    registered_attribution_.pop_front();

  registered_attribution_.emplace_back(v8::Isolate::GetCurrent(), error, path, line);
}

void ExceptionHandler::OnMessage(v8::Local<v8::Message> message, v8::Local<v8::Value> error, MessageSource source,
                                 v8::Local<v8::Promise> promise) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  if (isolate->IsExecutionTerminating())
    isolate->CancelTerminateExecution();

  auto context = isolate->GetEnteredOrMicrotaskContext();
  if (context.IsEmpty()) {
    LOG(ERROR) << "Unable to handle exception: no entered context";
    return;
  }

  if (!promise.IsEmpty()) {
    queued_messages_.emplace_back(isolate, message, error, source, promise);
    return;
  }

  runtime_delegate_->OnScriptOutput("=========================");

  std::string prefix;
  {
    // (1) Append the resource name.
    const base::FilePath& source_directory = runtime_->source_directory();

    std::string resource_name = toString(message->GetScriptResourceName());
    int resource_line = message->GetLineNumber(context).ToChecked();

    // Try to correct the |resource_name| and |resource_line| with saved attribution.
    if (resource_name == "undefined" || !resource_line) {
      for (const auto& attribution : registered_attribution_) {
        if (attribution.error != error)
          continue;

        resource_name = attribution.path.value();
        resource_line = attribution.line;
        break;
      }
    }

    if (!resource_name.find(source_directory.value()))
      prefix += resource_name.substr(source_directory.value().length() + 1);
    else
      prefix += resource_name;

    // (2) Append the line number.
    prefix += ":" + std::to_string(resource_line);
    prefix += ": ";
  }

  if (!message.IsEmpty() && message->Get()->Length() > 0) {
    runtime_delegate_->OnScriptOutput("JavaScript exception: " + toString(message->Get()));
    runtime_delegate_->OnScriptOutput(" ");  // blank line
  }

  if (!error.IsEmpty()) {
    v8::MaybeLocal<v8::String> error_string_maybe = error->ToString(context);
    v8::Local<v8::String> error_string;

    if (error_string_maybe.ToLocal(&error_string))
      runtime_delegate_->OnScriptOutput(prefix + toString(error_string));
  }

  v8::Local<v8::StackTrace> stack_trace = message->GetStackTrace();

  for (int frame = 0; frame < stack_trace->GetFrameCount(); ++frame) {
    v8::Local<v8::StackFrame> stack_frame = stack_trace->GetFrame(isolate, frame);
    v8::Local<v8::String> function_name = stack_frame->GetFunctionName();

    std::string function = kAnonymousFunction;

    if (!function_name.IsEmpty() && !function_name->IsNullOrUndefined())
      function = toString(function_name);

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
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

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
