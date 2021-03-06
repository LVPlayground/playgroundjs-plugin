// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/provided_natives.h"

#include "base/logging.h"
#include "bindings/runtime_operations.h"
#include "bindings/utilities.h"

namespace bindings {
namespace {

ProvidedNatives* g_provided_natives = nullptr;

}  // namespace

ProvidedNatives::ProvidedNatives() {
  DCHECK(!g_provided_natives);
  g_provided_natives = this;
}

ProvidedNatives::~ProvidedNatives() {
  g_provided_natives = nullptr;
}

// static
ProvidedNatives* ProvidedNatives::GetInstance() {
  return g_provided_natives;
}

void ProvidedNatives::SetNatives(const std::vector<std::string>& natives) {
  for (const std::string& native : natives)
    known_natives_.insert(native);
}

bool ProvidedNatives::Register(const std::string& name, const std::string& signature, v8::Local<v8::Function> fn) {
  if (known_natives_.find(name) == known_natives_.end())
    return false;  // the native named |name| is not known.

  StoredNative native;
  native.param_count = 0;
  native.retval_count = 0;

  for (size_t i = 0; i < signature.size(); ++i) {
    switch (signature[i]) {
    case 'f':
    case 'i':
    case 's':
      native.param_count++;
      break;
    case 'F':
    case 'I':
    case 'S':
      native.retval_count++;
      break;
    default:
      return false;  // unrecognized character in the signature.
    }
  }

  native.name = name;
  native.signature = signature;
  native.reference = v8::Persistent<v8::Function>(v8::Isolate::GetCurrent(), fn);

  native_handlers_[name] = native;
  return true;
}

int32_t ProvidedNatives::Call(const std::string& name, plugin::NativeParameters& params) {
  auto native_iter = native_handlers_.find(name);
  if (native_iter == native_handlers_.end()) {
    LOG(WARNING) << "No JavaScript listener has been defined for the " << name << " native.";
    return 0;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  auto context = isolate->GetCurrentContext();

  const StoredNative& native = native_iter->second;
  if (params.count() < native.param_count)
    return 0;  // not enough parameters.

  v8::HandleScope scope(isolate);

  std::vector<v8::Local<v8::Value>> arguments(native.param_count);

  size_t param_index = 0;
  for (size_t i = 0; i < native.signature.size(); ++i) {
    switch (native.signature[i]) {
    case 'f':
      arguments[param_index++] = v8::Number::New(isolate, static_cast<double>(params.GetFloat(i)));
      break;
    case 'i':
      arguments[param_index++] = v8::Number::New(isolate, static_cast<double>(params.GetInteger(i)));
      break;
    case 's':
      arguments[param_index++] = v8String(params.GetString(i, &text_buffer_));
      break;
    }
  }

  CHECK(param_index == native.param_count);

  int return_value = 1;
  {
    if (native.reference.IsEmpty()) {
      LOG(WARNING) << "[v8] Empty function found for native " << native.name;
      return 0;
    }

    v8::Local<v8::Function> function = native.reference.Get(isolate);
    if (function.IsEmpty()) {
      LOG(WARNING) << "[v8] Unable to coerce the persistent funtion to a local for native " << native.name;
      return 0;
    }

    v8::Local<v8::Value> value = bindings::Call(isolate, function, arguments.data(), arguments.size());
    if (value.IsEmpty())
      value = v8::Number::New(isolate, 0 /* default value */);

    if (native.retval_count == 0 && value->IsInt32()) {
      return_value = value->Int32Value(context).ToChecked();
    } else if (native.retval_count > 0) {
      if (!value->IsArray())
        return -1;  // reference values must be returned in an array

      v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(value);
      if (arr->Length() != native.retval_count)
        return -1;  // a wrong amount of reference values has been returned

      size_t retval_index = 0;
      for (size_t i = 0; i < native.signature.size(); ++i) {
        if (native.signature[i] != 'I' && native.signature[i] != 'F' && native.signature[i] != 'S')
          continue;

        v8::Local<v8::Value> retval = arr->Get(context, retval_index++).ToLocalChecked();
        if (retval.IsEmpty()) {
          LOG(WARNING) << "[v8] Unable to read return values of " << native.name << ": parameter "
                       << i << " not set.";

          // Always resort to setting the default value to whatever Pawn expects.
          retval = v8::Null(isolate);
        }

        switch (native.signature[i]) {
        case 'F':
          if (retval->IsNumber())
            params.SetFloat(i, static_cast<float>(retval->NumberValue(context).ToChecked()));
          else
            params.SetFloat(i, -1);
          break;
        case 'I':
          if (retval->IsInt32())
            params.SetInteger(i, retval->Int32Value(context).ToChecked());
          else
            params.SetInteger(i, -1);
          break;
        case 'S':
          if (retval->IsString()) {
            v8::String::Utf8Value text_value(isolate, retval);
            if (text_value.length()) {
              params.SetString(i, *text_value, text_value.length() + 1);
              break;
            }
          }

          params.SetString(i, "", 1);  // the empty string
          break;
        }
      }

      CHECK(retval_index == native.retval_count);
    }
  }

  return return_value;
}

}  // namespace bindings
