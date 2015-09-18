// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime_operations.h"

namespace bindings {

v8::Local<v8::Value> Call(v8::Isolate* isolate,
                          v8::Local<v8::Function> function,
                          v8::Local<v8::Value>* arguments,
                          size_t argument_count) {
  // Create an escapable handle scope in case the call creates tons of 'em.
  v8::EscapableHandleScope escapable_scope(isolate);

  // Create a TryCatch block to catch any exceptions that may be thrown by |function|. These will
  // be outputted to the console by the Runtime, but don't stop further event handlers.
  v8::TryCatch try_catch;

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::MaybeLocal<v8::Value> result = function->Call(context, context->Global(),
                                                    argument_count, arguments);

  if (result.IsEmpty())
    return v8::Local<v8::Value>();

  return escapable_scope.Escape(result.ToLocalChecked());
}

}  // namespace bindings
