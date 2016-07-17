// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_PROMISE_H_
#define PLAYGROUND_BINDINGS_PROMISE_H_

#include <string>

#include <include/v8.h>

#include "base/macros.h"

namespace bindings {

// Encapsulates functionality for a v8 Promise. Keep a reference to the Promise instance around
// (we suggest using std::unique_ptr<> when possible) to access the Resolve() and Reject() methods,
// and return GetPromise() to the script so that it can interact with the promise.
class Promise {
 public:
  Promise();
  ~Promise();

  // Returns the Promise object the script can use to observe the operation.
  v8::Local<v8::Promise> GetPromise() const;

  // Returns whether the promise has settled. Further calls to Resolve() and Reject() will fail
  // when this is the case, as a promise can only settle once.
  bool HasSettled() const { return has_settled_; }

  // Resolves the promise with |value|. The promise must not have been settled yet. The supported
  // types are listed further down in this header file.
  template<typename T> bool Resolve(T value);

  // Rejects the promise with |value|. The promise must not have been settled yet. The supported
  // types are listed further down in this header file.
  template<typename T> bool Reject(T value);

 private:
  bool ResolveInternal(v8::Local<v8::Value> value);
  bool RejectInternal(v8::Local<v8::Value> value);

  v8::Persistent<v8::Promise::Resolver> resolver_;
  bool has_settled_;

  DISALLOW_COPY_AND_ASSIGN(Promise);
};

template<> inline bool Promise::Resolve(v8::Local<v8::Value> value) { return ResolveInternal(value); }
template<> inline bool Promise::Reject(v8::Local<v8::Value> value) { return RejectInternal(value); }

template<> inline bool Promise::Resolve(v8::Local<v8::Object> value) { return ResolveInternal(value); }
template<> inline bool Promise::Reject(v8::Local<v8::Object> value) { return RejectInternal(value); }

template<> inline bool Promise::Resolve(v8::Local<v8::Array> value) { return ResolveInternal(value); }
template<> inline bool Promise::Reject(v8::Local<v8::Array> value) { return RejectInternal(value); }

template<> inline bool Promise::Resolve(bool value) {
  return ResolveInternal(v8::Boolean::New(v8::Isolate::GetCurrent(), value));
}

template<> inline bool Promise::Reject(bool value) {
  return RejectInternal(v8::Boolean::New(v8::Isolate::GetCurrent(), value));
}

template<> inline bool Promise::Resolve(const std::string& value) {
  v8::MaybeLocal<v8::String> string = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), value.c_str(), v8::NewStringType::kNormal);
  if (string.IsEmpty())
    return false;

  return ResolveInternal(string.ToLocalChecked());
}

template<> inline bool Promise::Reject(const std::string& value) {
  v8::MaybeLocal<v8::String> string = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), value.c_str(), v8::NewStringType::kNormal);
  if (string.IsEmpty())
    return false;

  return RejectInternal(string.ToLocalChecked());
}

#define DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(type) \
  template<> inline bool Promise::Resolve(type value) { \
    return ResolveInternal(v8::Number::New(v8::Isolate::GetCurrent(), static_cast<double>(value))); \
  } \
  template<> inline bool Promise::Reject(type value) { \
    return RejectInternal(v8::Number::New(v8::Isolate::GetCurrent(), static_cast<double>(value))); \
  }

DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(char)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(unsigned char)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(short)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(unsigned short)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(int)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(unsigned int)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(long)
DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE(unsigned long)

#undef DEFINE_RESOLVE_REJECT_FOR_NUMBER_TYPE

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_PROMISE_H_