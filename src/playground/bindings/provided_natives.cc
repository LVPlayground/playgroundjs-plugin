// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/provided_natives.h"

#include "base/logging.h"
#include "bindings/runtime_operations.h"

namespace bindings {
namespace {

ProvidedNatives* g_provided_natives = nullptr;

}  // namespace

ProvidedNatives::ProvidedNatives() {
  DCHECK(!g_provided_natives);
  g_provided_natives = this;

  natives_ = {
    {"TestFunction", Function::TestFunction}
  };
}

ProvidedNatives::~ProvidedNatives() {
  g_provided_natives = nullptr;
}

// static
ProvidedNatives* ProvidedNatives::GetInstance() {
  return g_provided_natives;
}

bool ProvidedNatives::Register(const std::string& name, const std::string& parameters, v8::Local<v8::Function> fn) {
  auto native_iter = natives_.find(name);
  if (native_iter == natives_.end())
    return false;  // the native named |name| is not known.

  StoredNative native;
  native.name = name;
  native.parameters = parameters;
  native.reference = v8::Persistent<v8::Function>(v8::Isolate::GetCurrent(), fn);

  native_handlers_.insert_or_assign(native_iter->second, native);
  return true;
}

int32_t ProvidedNatives::Call(Function fn, plugin::NativeParameters& params) {
  auto native_iter = native_handlers_.find(fn);
  if (native_iter == native_handlers_.end())
    return 0;  // the native for |fn| is not being listened to.

  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  const StoredNative& native = native_iter->second;
  const size_t argument_count = 1;

  v8::Local<v8::Value> arguments[argument_count];
  arguments[0] = v8::Undefined(isolate);

  // TODO(Russell): Handle |params| and |native.parameters|.

  int return_value = 1;
  {
    if (native.reference.IsEmpty()) {
      LOG(WARNING) << "[v8] Empty function found for native " << native.name;
      return 0;
    }

    v8::Local<v8::Function> function = v8::Local<v8::Function>::New(isolate, native.reference);
    if (function.IsEmpty()) {
      LOG(WARNING) << "[v8] Unable to coerce the persistent funtion to a local for native " << native.name;
      return 0;
    }

    v8::Local<v8::Value> value = bindings::Call(isolate, function, arguments, argument_count);
    if (value->IsInt32())
      return_value = value->Int32Value();
  }

  return return_value;
}

}  // namespace bindings
