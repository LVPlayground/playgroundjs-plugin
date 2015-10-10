// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_RUNTIME_OPERATIONS_H_
#define PLAYGROUND_BINDINGS_RUNTIME_OPERATIONS_H_

#include <include/v8.h>

namespace bindings {

class Runtime;

// Calls |function| on the |isolate|, optionally with |arguments|. When using arguments,
// the |argument_count| parameter must be set to the number of passed arguments. This function
// will return a handle to the return value of the |function|.
v8::Local<v8::Value> Call(v8::Isolate* isolate,
                          v8::Local<v8::Function> function,
                          v8::Local<v8::Value>* arguments = nullptr,
                          size_t argument_count = 0);

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_RUNTIME_OPERATIONS_H_
