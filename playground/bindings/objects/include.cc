// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/objects/include.h"

#include "base/logging.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

Include* g_include = nullptr;

}  // namespace

Include::Include(Runtime* runtime)
    : GlobalObject(runtime) {
  g_include = this;
}

Include::~Include() {
  g_include = nullptr;
}

// static
void Include::IncludeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (!g_include)
    return;

  if (arguments.Length() < 1 || !arguments[0]->IsString()) {
    ThrowException("unable to execute include(): 1 argument required, but only 0 present");
    return;
  }

  const std::string filename = toString(arguments[0]);
  if (!filename.size()) {
    ThrowException("unable to execute include(): unable to read the first argument as a string");
    return;
  }

  v8::Local<v8::Value> result_value;

  const bool result =
      g_include->runtime()->ExecuteFile(base::FilePath(filename),
                                        Runtime::EXECUTION_TYPE_MODULE,
                                        &result_value);

  if (!result)
    ThrowException("unable to execute include(): unable to execute `" + filename + "`");

  arguments.GetReturnValue().Set(result_value);
}

void Include::InstallPrototype(v8::Local<v8::ObjectTemplate> global) {
  global->Set(v8String("include"), v8::FunctionTemplate::New(isolate(), Include::IncludeCallback));
}

}  // namespace bindings
