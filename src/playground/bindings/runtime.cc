// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime.h"

#include <algorithm>
#include <chrono>
#include <string.h>
#include <thread>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <include/libplatform/libplatform.h>

#include "base/logging.h"
#include "base/time.h"
#include "bindings/exception_handler.h"
#include "bindings/frame_observer.h"
#include "bindings/global_scope.h"
#include "bindings/modules/streamer/streamer_host.h"
#include "bindings/profiler.h"
#include "bindings/runtime_modulator.h"
#include "bindings/timer_queue.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

// Context extensions that should be enabled on our v8 scope.
const int kContextExtensionCount = 1;
const char* kContextExtensions[] = {
  "v8/statistics",
};

// Map of v8 Isolates to the associated Runtime instances (weak references).
std::unordered_map<v8::Isolate*, std::weak_ptr<Runtime>> g_runtime_instances_;

// String of the v8 flags that should apply to this virtual machine. See flag-definitions.h in
// the v8 source code for a list of all supported command line flags.
const char kRuntimeFlags[] =
    "--expose_gc "
    "--use_strict "

    // Private methods and weak references
    "--harmony_intl_dateformat_day_period "
    "--harmony_intl_segmenter";

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
  if (runtime) {
    runtime->GetExceptionHandler()->OnMessage(
      message, error, ExceptionHandler::MessageSource::kScript);
  }
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
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(isolate);
  if (!runtime)
    return;

  auto promise = message.GetPromise();
  auto value = message.GetValue();

  if (message.GetEvent() == v8::kPromiseHandlerAddedAfterReject) {
    runtime->GetExceptionHandler()->RevokeQueuedMessages(promise);
    return;
  }

  v8::Local<v8::Message> error_message = v8::Exception::CreateMessage(isolate, value);
  if (error_message.IsEmpty())
    return;

  runtime->GetExceptionHandler()->OnMessage(
      error_message, value, ExceptionHandler::MessageSource::kRejectedPromise, promise);
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
      platform_(v8::platform::NewDefaultPlatform()),
      main_thread_io_context_(),
      background_io_context_(),
      background_thread_guard_(boost::asio::make_work_guard(background_io_context_)),
      background_thread_(std::make_unique<boost::thread>(boost::bind(&boost::asio::io_context::run,
                                                                     &background_io_context_))),
      is_ready_(false),
      frame_counter_start_(::base::monotonicallyIncreasingTime()),
      frame_counter_(0) {
  v8::V8::InitializeICU("icudtl.dat");
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

  streamer_host_ = std::make_unique<streamer::StreamerHost>(
      plugin_controller, main_thread_io_context_, background_io_context_);

  source_directory_ = base::FilePath::CurrentDirectory().Append("javascript");
}

Runtime::~Runtime() {
  background_thread_guard_.reset();

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

  v8::ExtensionConfiguration configuration(kContextExtensionCount, kContextExtensions);
  v8::Local<v8::Context> context = v8::Context::New(isolate_, &configuration, global);

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

  if (main_thread_io_context_.stopped())
    main_thread_io_context_.restart();

  main_thread_io_context_.run_one();

  streamer_host_->OnFrame(current_time);

  if (exception_handler_->HasQueuedMessages()) {
    v8::HandleScope handle_scope(isolate_);
    v8::Context::Scope context_scope(context());

    exception_handler_->FlushMessageQueue();
  }
}

void Runtime::AddFrameObserver(FrameObserver* observer) {
  frame_observers_.insert(observer);
}

void Runtime::RemoveFrameObserver(FrameObserver* observer) {
  frame_observers_.erase(observer);
}

}  // namespace bindings
