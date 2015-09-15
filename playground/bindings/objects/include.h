// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_OBJECTS_INCLUDE_H_
#define PLAYGROUND_BINDINGS_OBJECTS_INCLUDE_H_

#include "bindings/global_object.h"

namespace bindings {

class Runtime;

// The Include class provides the include() function, providing the ability to load other files.
class Include : public GlobalObject {
 public:
  explicit Include(Runtime* runtime);
  ~Include();

  // Callbacks for the global include() function. It will synchronously load the file indicated in
  // the first argument, and return the value of the |exports| global specific to its context.
  static void IncludeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);

  // GlobalObject implementation.
  void InstallPrototype(v8::Local<v8::ObjectTemplate> global) override;
};

}

#endif  // PLAYGROUND_BINDINGS_OBJECTS_INCLUDE_H_
