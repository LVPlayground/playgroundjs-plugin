// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/global_callbacks.h"

#include "base/file_path.h"
#include "base/file_search.h"
#include "base/logging.h"
#include "base/memory.h"
#include "base/time.h"
#include "bindings/event.h"
#include "bindings/exception_handler.h"
#include "bindings/global_scope.h"
#include "bindings/modules/execute.h"
#include "bindings/pawn_invoke.h"
#include "bindings/promise.h"
#include "bindings/runtime.h"
#include "bindings/runtime_modulator.h"
#include "bindings/timer_queue.h"
#include "bindings/utilities.h"
#include "performance/trace_manager.h"
#include "plugin/sdk/plugincommon.h"

#include <include/v8.h>
#include <string>

namespace bindings {

// void addEventListener(string type, function listener);
void AddEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() < 2) {
    ThrowException("unable to execute addEventListener(): 2 arguments required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute addEventListener(): expected a string for argument 1.");
    return;
  }

  if (!arguments[1]->IsFunction()) {
    ThrowException("unable to execute addEventListener(): expected a function for argument 2.");
    return;
  }

  global->AddEventListener(toString(arguments[0]), v8::Local<v8::Function>::Cast(arguments[1]));
}

// void clearModuleCache(string prefix);
void ClearModuleCacheCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto runtime = Runtime::FromIsolate(arguments.GetIsolate());

  if (arguments.Length() < 1) {
    ThrowException("unable to execute clearModuleCache(): 1 arguments required, but none provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute clearModuleCache(): expected a string for argument 1.");
    return;
  }

  runtime->GetModulator()->ClearCache(toString(arguments[0]));
}

// boolean dispatchEvent(string type[, object event]);
void DispatchEventCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute dispatchEvent(): 1 argument required, but only 0 provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute dispatchEvent(): expected a string for argument 1.");
    return;
  }

  const std::string type = toString(arguments[0]);

  if (arguments.Length() >= 2)
    global->DispatchEvent(type, arguments[1]);
  else
    global->DispatchEvent(type, v8::Null(arguments.GetIsolate()));
}

// Promise<{ exitCode, output, error }> exec(string command, ...arguments);
void ExecCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(arguments.GetIsolate());

  if (!arguments.Length()) {
    ThrowException("unable to execute exec(): 1 arguments required, but none provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute exec(): expected a string for argument 1.");
    return;
  }

  std::string command = toString(arguments[0]);
  std::vector<std::string> args;

  for (int i = 1; i < arguments.Length(); ++i) {
    if (!arguments[i]->IsString()) {
      ThrowException("unable to execute exec(): expected a string for argument " +
                     std::to_string(i + 1) + ".");
      return;
    }

    args.push_back(toString(arguments[i]));
  }

  std::shared_ptr<Promise> promise = std::make_shared<Promise>();

  Execute(runtime->main_thread_io_context(), command, args,
          [promise](int exit_code, const std::string& output, const std::string& error) {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();

            v8::HandleScope handle_scope(isolate);

            v8::Local<v8::Context> context = Runtime::FromIsolate(isolate)->context();
            v8::Context::Scope context_scope(context);

            v8::Local<v8::Object> object = v8::Object::New(isolate);

            object->Set(context, v8String("exitCode"), v8::Number::New(isolate, exit_code));
            object->Set(context, v8String("output"), v8String(output));
            object->Set(context, v8String("error"), v8String(error));

            promise->Resolve(object);
          });

  arguments.GetReturnValue().Set(promise->GetPromise());
}

// object { duration, fps } frameCounter();
void FrameCounterCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(arguments.GetIsolate());
  auto context = runtime->context();

  double duration, average_fps;
  runtime->GetAndResetFrameCounter(&duration, &average_fps);

  v8::Local<v8::Object> object = v8::Object::New(runtime->isolate());
  object->Set(context, v8String("duration"), v8::Number::New(runtime->isolate(), duration));
  object->Set(context, v8String("fps"), v8::Number::New(runtime->isolate(), average_fps));

  arguments.GetReturnValue().Set(object);
}

// void flushExceptionQueue();
void FlushExceptionQueueCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(arguments.GetIsolate());
  auto exception_handler = runtime->GetExceptionHandler();

  if (exception_handler->HasQueuedMessages())
    exception_handler->FlushMessageQueue();
}

// sequence<object { type, event }> getDeferredEvents();
void GetDeferredEventsCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  v8::Isolate* isolate = arguments.GetIsolate();
  auto context = isolate->GetCurrentContext();

  GlobalScope* global = Runtime::FromIsolate(isolate)->GetGlobalScope();
  GlobalScope::DeferredEventMultimapType& deferred_events = global->deferred_events();

  v8::Local<v8::Array> events = v8::Array::New(isolate, deferred_events.size());
  v8::Local<v8::Name> names[] = { v8String("type"), v8String("event") };

  uint32_t index = 0;
  for (auto& [type, arguments] : deferred_events) {
    Event* event = global->GetEvent(type);
    if (!event) {
      LOG(ERROR) << "Unrecognized event name: " << type << ". Dropping deferred event.";
      continue;
    }

    v8::Local<v8::Value> event_values[] = { v8String(type), event->NewInstance(arguments) };

    events->Set(
        context, index++, v8::Object::New(isolate, v8::Null(isolate), names, event_values, 2));
  }

  deferred_events.clear();

  arguments.GetReturnValue().Set(events);
}

#define ADD_NUMBER(name, value) \
    object->Set(context, v8String(name), v8::Number::New(isolate, value))

// object getRuntimeStatistics();
void GetRuntimeStatisticsCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  v8::Isolate* isolate = arguments.GetIsolate();
  auto context = isolate->GetCurrentContext();

  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(isolate);
  auto* global = runtime->GetGlobalScope();

  v8::Local<v8::Object> object = v8::Object::New(isolate);

  ADD_NUMBER("deferred_event_queue_size", global->deferred_events().size());
  ADD_NUMBER("event_handler_size", global->event_handler_count());
  ADD_NUMBER("exception_handler_queue_size", runtime->GetExceptionHandler()->size());
  ADD_NUMBER("timer_queue_size", runtime->GetTimerQueue()->size());

  arguments.GetReturnValue().Set(object);
}

// sequence<string> glob(string base, string pattern);
void GlobCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (arguments.Length() < 2) {
    ThrowException("unable to execute glob(): 2 arguments required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute glob(): expected a string for argument 1.");
    return;
  }

  if (!arguments[1]->IsString()) {
    ThrowException("unable to execute glob(): expected a string for argument 2.");
    return;
  }

  v8::Isolate* isolate = arguments.GetIsolate();
  auto context = isolate->GetCurrentContext();

  const base::FilePath base = base::FilePath::CurrentDirectory().Append(toString(arguments[0]));
  const std::string query = toString(arguments[1]);

  std::vector<std::string> results;

  FileSearchStatus status = FileSearch(base, query, &results);
  switch (status) {
  case FileSearchStatus::ERR_INVALID_REGEX:
    ThrowException("unable to execute glob(): invalid expression: " + query);
    break;
  case FileSearchStatus::SUCCESS:
    {
      v8::Local<v8::Array> arr = v8::Array::New(isolate, results.size());
      for (uint32_t i = 0; i < results.size(); ++i)
        arr->Set(context, i, v8String(results[i]));

      arguments.GetReturnValue().Set(arr);
    }
    break;
  }
}

// boolean hasEventListeners(string type);
void HasEventListenersCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute hasEventListeners(): 1 argument required, but only 0 provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute hasEventListeners(): expected a string for argument 1.");
    return;
  }

  arguments.GetReturnValue().Set(global->HasEventListeners(toString(arguments[0])));
}

// double highResolutionTime();
void HighResolutionTimeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();
  arguments.GetReturnValue().Set(global->HighResolutionTime());
}

// bool isPlayerMinimized(playerId [, currentTime]);
void IsPlayerMinimizedCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto runtime = Runtime::FromIsolate(arguments.GetIsolate());

  auto context = runtime->context();
  auto global = runtime->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute isPlayerMinimized(): 1 argument required, but only 0 provided.");
    return;
  }

  if (!arguments[0]->IsInt32()) {
    ThrowException("unable to execute isPlayerMinimized(): expected an integer for argument 1.");
    return;
  }

  double current_time = 0;

  if (arguments.Length() >= 2 && arguments[1]->IsNumber())
    current_time = arguments[1]->NumberValue(context).ToChecked();
  else
    current_time = base::monotonicallyIncreasingTime();

  arguments.GetReturnValue().Set(
    global->IsPlayerMinimized(arguments[0]->Int32Value(context).ToChecked(), current_time));
}

// void notifyReady();
void NotifyReadyCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  Runtime::FromIsolate(arguments.GetIsolate())->SetReady();
}

// any pawnInvoke(string name[, string signature[, ...]]);
void PawnInvokeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute pawnInvoke(): 1 argument required, but 0 provided.");
    return;
  }

  arguments.GetReturnValue().Set(global->GetPawnInvoke()->Call(arguments));
}

// void provideNative(string name, string parameters, function handler);
void ProvideNativeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (!pAMXFunctions) {
    ThrowException("unable to register natives in the test runner.");
    return;
  }

  if (arguments.Length() != 3) {
    ThrowException("unable to execute provideNative(): 3 argument required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute provideNative(): expected a string for argument 1.");
    return;
  }

  if (!arguments[1]->IsString()) {
    ThrowException("unable to execute provideNative(): expected a string for argument 2.");
    return;
  }

  if (!arguments[2]->IsFunction()) {
    ThrowException("unable to execute provideNative(): expected a function for argument 3.");
    return;
  }

  const std::string name = toString(arguments[0]);
  const std::string parameters = toString(arguments[1]);

  if (!global->GetProvidedNatives()->Register(name, parameters, v8::Local<v8::Function>::Cast(arguments[2])))
    ThrowException("unable to execute provideNative(): the native could not be registered.");
}

// string readFile(string filename);
void ReadFileCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute readFile(): 1 argument required, but 0 provided.");
    return;
  }

  arguments.GetReturnValue().Set(v8String(global->ReadFile(toString(arguments[0]))));
}

// void removeEventListener(string type[, function listener]);
void RemoveEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute removeEventListener(): 1 argument required, but 0 provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute removeEventListener(): expected a string for argument 1.");
    return;
  }

  v8::Local<v8::Function> function;
  if (arguments[1]->IsFunction())
    function = v8::Local<v8::Function>::Cast(arguments[1]);

  global->RemoveEventListener(toString(arguments[0]), function);
}

// void reportTestsFinished(int totalTests, int failedTests);
void ReportTestsFinishedCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (arguments.Length() != 2) {
    ThrowException("unable to execute reportTestsFinished(): 2 argument required, but only" +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsNumber() || !arguments[1]->IsNumber()) {
    ThrowException("unable to execute reportTestsFinished(): expected numbers as arguments.");
    return;
  }

  auto context = GetContext();

  unsigned int total_tests = static_cast<unsigned int>(GetInt64(context, arguments[0]));
  unsigned int failed_tests = static_cast<unsigned int>(GetInt64(context, arguments[1]));

  auto runtime = Runtime::FromIsolate(arguments.GetIsolate());

  runtime->GetGlobalScope()->VerifyNoEventHandlersLeft();

  Runtime::Delegate* runtime_delegate = runtime->delegate();
  if (runtime_delegate)
    runtime_delegate->OnScriptTestsDone(total_tests, failed_tests);

  if (!pAMXFunctions) {
    runtime->SetReady();  // this stops the plugin from spinning
    ThrowException("The Test Runner is done- all's good, thanks for using this tool!");
  }
}

// void killServer();
void KillServerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
#if defined(WIN32)
  abort();
#else
  exit(-1);
#endif
}

// void startTrace();
void StartTraceCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  LOG(INFO) << "[TraceManager] Started capturing traces.";
  performance::TraceManager::GetInstance()->set_enabled(true);
}

// void stopTrace(optional string filename);
void StopTraceCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  LOG(INFO) << "[TraceManager] Stopped capturing traces.";
  performance::TraceManager::GetInstance()->set_enabled(false);

  if (arguments.Length() == 0)
    return;

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute stopTrace(): expected a string for argument 1.");
    return;
  }

  const std::string filename = toString(arguments[0]);
  if (!filename.size()) {
    ThrowException("unable to execute stopTrace(): expected a non-empty string for argument 1.");
    return;
  }
  
  const base::FilePath file = base::FilePath::CurrentDirectory().Append(filename);

  // Write the captured traces to the |path|, clear state afterwards.
  performance::TraceManager::GetInstance()->Write(file, true /* clear_traces */);
}

// void toggleMemoryLogging();
void ToggleMemoryLoggingCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  base::g_debugMemoryAllocations = !base::g_debugMemoryAllocations;
}

// Promise<void> wait(unsigned long time);
void WaitCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(arguments.GetIsolate());
  GlobalScope* global = runtime->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute wait(): 1 argument required, but only 0 provided.");
    return;
  }

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to execute wait(): expected a number for argument 1.");
    return;
  }

  arguments.GetReturnValue().Set(global->Wait(runtime.get(), GetInt64(runtime->context(), arguments[0])));
}

}  // namespace bindings
