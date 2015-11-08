// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/promise.h"

#include "base/logging.h"

namespace bindings {

Promise::Promise()
    : has_settled_(false) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::MaybeLocal<v8::Promise::Resolver> maybe_resolver = v8::Promise::Resolver::New(context);
  v8::Local<v8::Promise::Resolver> resolver;

  if (!maybe_resolver.ToLocal(&resolver)) {
    LOG(ERROR) << "Unable to create a new promise: Resolver::New() failed.";
    return;
  }

  resolver_.Reset(isolate, resolver);
}

Promise::~Promise() {
  resolver_.Reset();
}

v8::Local<v8::Promise> Promise::GetPromise() const {
  DCHECK(!resolver_.IsEmpty());
  return resolver_.Get(v8::Isolate::GetCurrent())->GetPromise();
}

bool Promise::ResolveInternal(v8::Local<v8::Value> value) {
  if (has_settled_)
    return false;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Maybe<bool> result = resolver_.Get(isolate)->Resolve(context, value);
  if (result.IsNothing() || !result.FromJust())
    LOG(ERROR) << "Resolving the promise has failed.";

  return has_settled_ = true;
}

bool Promise::RejectInternal(v8::Local<v8::Value> value) {
  if (has_settled_)
    return false;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Maybe<bool> result = resolver_.Get(isolate)->Reject(context, value);
  if (result.IsNothing())
    LOG(ERROR) << "Rejecting the promise has failed.";

  return has_settled_ = true;
}

}  // namespace bindings
