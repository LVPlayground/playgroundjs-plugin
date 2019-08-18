// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_UTILITIES_H_
#define PLAYGROUND_BINDINGS_UTILITIES_H_

#include <string>

#include <include/v8.h>

#include "bindings/exception_handler.h"

namespace bindings {

inline v8::Isolate* GetIsolate() {
  return v8::Isolate::GetCurrent();
}

// Defined in runtime.cc to avoid a circular dependency.
v8::Local<v8::Context> GetContext();

inline int64_t GetInt64(v8::Local<v8::Context> context, v8::Local<v8::Value> source) {
  return source->ToNumber(context).ToLocalChecked()->IntegerValue(context).ToChecked();
}

inline v8::Local<v8::String> v8String(const char* string, int length) {
  return v8::String::NewFromUtf8(GetIsolate(), string, v8::NewStringType::kNormal, length).ToLocalChecked();
}

inline v8::Local<v8::String> v8String(const char* string) {
  return v8String(string, strlen(string));
}

inline v8::Local<v8::String> v8String(const std::string& string) {
  return v8::String::NewFromUtf8(GetIsolate(), string.c_str(), v8::NewStringType::kNormal, string.size()).ToLocalChecked();
}

inline v8::Local<v8::Primitive> v8Null() {
  return v8::Null(GetIsolate());
}

template<typename T>
inline v8::Local<v8::Number> v8Number(T number) {
  return v8::Number::New(GetIsolate(), static_cast<double>(number));
}

inline std::string toString(v8::Local<v8::Value> string) {
  v8::String::Utf8Value value(GetIsolate(), string);
  if (value.length())
    return std::string(*value, value.length());

  return std::string();
}

inline void ThrowException(const std::string& message) {
  v8::Local<v8::Value> error = v8::Exception::TypeError(v8String(message));
  if (ScopedExceptionAttribution::HasAttribution())
    RegisterError(error);

  v8::Isolate::GetCurrent()->ThrowException(error);
}

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_UTILITIES_H_
