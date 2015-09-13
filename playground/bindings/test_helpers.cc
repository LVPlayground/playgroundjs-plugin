// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/test_helpers.h"

#include <include/v8.h>

#include "bindings/runtime.h"
#include "bindings/runtime_options.h"

namespace bindings {

// static
bool ScriptRunner::Execute(const RuntimeOptions& options, const std::string& source) {
  Runtime runtime(options);

  Runtime::ScriptSource script;
  script.source = source;

  v8::HandleScope handle_scope(runtime.isolate());
  v8::Local<v8::Value> result;

  if (!runtime.Execute(script, &result))
    return false;

  if (result->IsBoolean())
    return result.As<v8::Boolean>()->Value();

  // TODO: Log a warning when the return value can't be converted.
  return false;
}

// static
int32_t ScriptRunner::ExecuteInt(const RuntimeOptions& options, const std::string& source) {
  Runtime runtime(options);

  Runtime::ScriptSource script;
  script.source = source;

  v8::HandleScope handle_scope(runtime.isolate());
  v8::Local<v8::Value> result;

  if (!runtime.Execute(script, &result))
    return false;

  if (result->IsInt32())
    return result.As<v8::Int32>()->Value();

  // TODO: Log a warning when the return value can't be converted.
  return -1;
}

// static
std::string ScriptRunner::ExecuteString(const RuntimeOptions& options, const std::string& source) {
  Runtime runtime(options);

  Runtime::ScriptSource script;
  script.source = source;

  v8::HandleScope handle_scope(runtime.isolate());
  v8::Local<v8::Value> result;

  if (!runtime.Execute(script, &result))
    return false;

  v8::String::Utf8Value string(result);
  if (!string.length())
    return std::string();

  return std::string(*string, string.length());
}

}  // namespace bindings
