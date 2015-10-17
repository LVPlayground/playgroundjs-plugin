// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/global_callbacks.h"

#include "bindings/global_scope.h"
#include "bindings/pawn_invoke.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

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

// any pawnInvoke(string name[, string signature[, ...]]);
void PawnInvokeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute pawnInvoke(): 1 argument required, but 0 provided.");
    return;
  }

  arguments.GetReturnValue().Set(global->GetPawnInvoke()->Call(arguments));
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

// object requireImpl(string filename);
// NOTE: The public entry-point is require(), so reflect this in exceptions.
void RequireImplCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(arguments.GetIsolate());
  GlobalScope* global = runtime->GetGlobalScope();

  if (arguments.Length() == 0) {
    ThrowException("unable to execute require(): 1 argument required, but only 0 provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to execute require(): expected a string for argument 1.");
    return;
  }

  arguments.GetReturnValue().Set(global->RequireImpl(runtime.get(), toString(arguments[0])));
}

}  // namespace bindings
