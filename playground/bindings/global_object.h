// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_OBJECT_H_
#define PLAYGROUND_BINDINGS_GLOBAL_OBJECT_H_

#include <string>

#include <include/v8.h>

#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {

class GlobalObject {
 public:
  explicit GlobalObject(Runtime* runtime)
      : runtime_(runtime) {}

  virtual ~GlobalObject() {}

  virtual void InstallPrototype(v8::Local<v8::ObjectTemplate> global) {}
  virtual void InstallObjects(v8::Local<v8::Object> global) {}

 protected:
  v8::Local<v8::Context> context() { return runtime_->context(); }
  v8::Isolate* isolate() { return runtime_->isolate(); }
  Runtime* runtime() { return runtime_; }

 private:
  Runtime* runtime_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_GLOBAL_OBJECT_H_
