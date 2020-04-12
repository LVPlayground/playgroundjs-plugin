// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime.h"

#include <algorithm>
#include <chrono>
#include <string.h>
#include <thread>
#include <unordered_map>

#include <include/libplatform/libplatform.h>

#include "base/logging.h"
#include "base/time.h"
#include "bindings/exception_handler.h"
#include "bindings/frame_observer.h"
#include "bindings/global_scope.h"
#include "bindings/profiler.h"
#include "bindings/runtime_modulator.h"
#include "bindings/timer_queue.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

// Map of v8 Isolates to the associated Runtime instances (weak references).
std::unordered_map<v8::Isolate*, std::weak_ptr<Runtime>> g_runtime_instances_;

// String of the v8 flags that should apply to this virtual machine. See flag-definitions.h in
// the v8 source code for a list of all supported command line flags.
const char kRuntimeFlags[] =
    "--expose_gc "
    "--use_strict "

    // Private methods and weak references
    "--harmony_string_replaceall "
    "--harmony_weak_refs";

// Returns whether |character| represents a line break.
bool IsLineBreak(char character) {
  return character == '\n' || character == '\r';
}

// Returns a string with all line breaks removed from |line|.
std::string RemoveLineBreaks(std::string line) {
  line.erase(std::remove_if(line.begin(), line.end(), IsLineBreak), line.end());
  return line;
}

// Callback for the v8 message handler. Forward the call to the exception handler.
void MessageCallback(v8::Local<v8::Message> message, v8::Local<v8::Value> error) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (runtime)
    runtime->GetExceptionHandler()->OnMessage(
        message, error, ExceptionHandler::MessageSource::kScript);
}

// Callback for the v8 fatal error handler. Forward the call to the exception handler.
void FatalErrorCallback(const char* location, const char* message) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (runtime)
    runtime->GetExceptionHandler()->OnFatalError(location, message);
}

// Callback for uncaught promise rejection events. Output the exception message to the console
// after all, so that developers can pick up on the error and deal with it.
void PromiseRejectCallback(v8::PromiseRejectMessage message) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (!runtime)
    return;

  if (message.GetEvent() == v8::kPromiseHandlerAddedAfterReject) {
    runtime->GetExceptionHandler()->RevokeQueuedMessages(message.GetPromise());
    return;
  }

  v8::Local<v8::Value> value = message.GetValue();
  if (value.IsEmpty() || !value->IsNativeError())
    return;

  v8::Local<v8::Message> error_message = v8::Exception::CreateMessage(GetIsolate(), value);
  if (error_message.IsEmpty())
    return;

  runtime->GetExceptionHandler()->OnMessage(
      error_message, value, ExceptionHandler::MessageSource::kRejectedPromise, message.GetPromise());
}

}  // namespace

// Declared in utilities.h
v8::Local<v8::Context> GetContext() {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (runtime)
    return runtime->context();

  return v8::Local<v8::Context>();
}

// static
std::shared_ptr<Runtime> Runtime::FromIsolate(v8::Isolate* isolate) {
  const auto instance = g_runtime_instances_.find(isolate);
  if (instance == g_runtime_instances_.end())
    return nullptr;

  CHECK(!instance->second.expired());
  return instance->second.lock();
}

// static
std::shared_ptr<Runtime> Runtime::Create(Delegate* runtime_delegate,
                                         plugin::PluginController* plugin_controller) {
  auto instance = std::shared_ptr<Runtime>(
      new Runtime(runtime_delegate, plugin_controller));
  CHECK(instance->isolate());

  g_runtime_instances_[instance->isolate()] = instance;
  return instance;
}

Runtime::Runtime(Delegate* runtime_delegate,
                 plugin::PluginController* plugin_controller)
    : global_scope_(new GlobalScope(plugin_controller)),
      runtime_delegate_(runtime_delegate),
      is_ready_(false),
      frame_counter_start_(::base::monotonicallyIncreasingTime()),
      frame_counter_(0) {
  v8::V8::InitializeICU();

#if defined(__linux__)
  //
  // This is a work-around for a linking issue we're having on Linux that quite
  // likely isn't worth fixing. Make the following changes to the v8 checkout:
  //
  // v8/include/libplatform/libplatform.h
  //     V8_PLATFORM_EXPORT v8::Platform* lvpLinuxLinkerHack();
  //
  // v8/src/libplatform/default-platform.cc
  //    v8::Platform* lvpLinuxLinkerHack() { return NewDefaultPlatform().release(); }
  //
  platform_.reset(v8::platform::lvpLinuxLinkerHack());
#else
  platform_ = v8::platform::NewDefaultPlatform();
#endif
  v8::V8::InitializePlatform(platform_.get());

  v8::V8::SetFlagsFromString(kRuntimeFlags, sizeof(kRuntimeFlags));
  v8::V8::Initialize();

  allocator_.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();

  isolate_ = v8::Isolate::New(create_params);
  isolate_scope_.reset(new v8::Isolate::Scope(isolate_));

  exception_handler_.reset(new ExceptionHandler(this, runtime_delegate_));
  isolate_->SetCaptureStackTraceForUncaughtExceptions(true, 15);
  isolate_->AddMessageListener(MessageCallback);
  isolate_->SetFatalErrorHandler(FatalErrorCallback);
  isolate_->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
  isolate_->SetPromiseRejectCallback(PromiseRejectCallback);
  isolate_->SetHostImportModuleDynamicallyCallback(
      &RuntimeModulator::ImportModuleDynamicallyCallback);

  profiler_.reset(new Profiler(isolate_));
  timer_queue_.reset(new TimerQueue(this));

  source_directory_ = base::FilePath::CurrentDirectory().Append("javascript");
}

Runtime::~Runtime() {
  global_scope_.reset();
  timer_queue_.reset();
  modulator_.reset();

  isolate_scope_.reset();
  isolate_->Dispose();

  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
}

void Runtime::Initialize() {
  v8::HandleScope handle_scope(isolate_);

  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);

  // The global scope is being applied in two steps. First we install all the prototypes (which
  // essentially are functions that may be instantiated), then we install objects, which may
  // include instantiations of previously installed prototypes (e.g. the console global).
  global_scope_->InstallPrototypes(global);

  v8::Local<v8::Context> context = v8::Context::New(isolate_, nullptr, global);

  v8::Context::Scope context_scope(context);
  global_scope_->InstallObjects(context);
  global_scope_->Finalize();

  context_.Reset(isolate_, context);

  modulator_.reset(new RuntimeModulator(isolate_, source_directory_));
  modulator_->LoadModule(context, /* referrer= */ base::FilePath(), "main.js");
}

void Runtime::SpinUntilReady() {
  while (!is_ready_) {
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    OnFrame();
  }
}

void Runtime::SetReady() {
  is_ready_ = true;
}

void Runtime::GetAndResetFrameCounter(double* duration, double* average_fps) {
  DCHECK(duration && average_fps);

  double current = ::base::monotonicallyIncreasingTime();

  *duration = current - frame_counter_start_;
  *average_fps = frame_counter_ / (*duration / 1000);

  frame_counter_start_ = current;
  frame_counter_ = 0;
}

void Runtime::OnFrame() {
  ++frame_counter_;

  double current_time = ::base::monotonicallyIncreasingTime();

  for (FrameObserver* observer : frame_observers_)
    observer->OnFrame();

  if (profiler_->IsActive())
    profiler_->OnFrame(current_time);

  timer_queue_->Run(current_time);

  isolate_->RunMicrotasks();

  exception_handler_->FlushMessageQueue();
}

void Runtime::AddFrameObserver(FrameObserver* observer) {
  frame_observers_.insert(observer);
}

void Runtime::RemoveFrameObserver(FrameObserver* observer) {
  frame_observers_.erase(observer);
}

}  // namespace bindings
