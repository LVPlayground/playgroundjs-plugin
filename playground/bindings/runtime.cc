// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime.h"

#include <include/libplatform/libplatform.h>

#include "base/logging.h"
#include "bindings/allocator.h"
#include "bindings/global_scope.h"
#include "bindings/runtime_options.h"

using namespace v8;

namespace bindings {

Runtime::Runtime(const RuntimeOptions& options, Delegate* delegate)
    : delegate_(delegate) {
  V8::InitializeICU();

  platform_.reset(platform::CreateDefaultPlatform());
  V8::InitializePlatform(platform_.get());

  std::string option_string = RuntimeOptionsToArgumentString(options);
  if (option_string.length())
    V8::SetFlagsFromString(option_string.c_str(), static_cast<int>(option_string.length()));

  V8::Initialize();

  allocator_.reset(new SimpleArrayBufferAllocator());

  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();

  isolate_ = Isolate::New(create_params);
  isolate_scope_.reset(new Isolate::Scope(isolate_));

  global_scope_.reset(new GlobalScope(this));
}

Runtime::~Runtime() {
  isolate_scope_.reset();
  isolate_->Dispose();

  V8::Dispose();
  V8::ShutdownPlatform();
}

void Runtime::Initialize() {
  HandleScope handle_scope(isolate_);

  Local<ObjectTemplate> global = ObjectTemplate::New(isolate_);

  // The global scope is being applied in two steps. First we install all the prototypes (which
  // essentially are functions that may be instantiated), then we install objects, which may
  // include instantiations of previously installed prototypes (e.g. the console global).
  global_scope_->InstallPrototypes(global);

  Local<Context> context = Context::New(isolate_, nullptr, global);

  Context::Scope context_scope(context);
  global_scope_->InstallObjects(context->Global());

  context_.Reset(isolate_, context);
}

bool Runtime::Execute(const ScriptSource& script_source, v8::Local<v8::Value>* result) {
  EscapableHandleScope handle_scope(isolate());
  Context::Scope context_scope(context());

  // TODO: Check that a HandleScope has been created by the caller.
  // TODO: Prepend and append module boiler-plate before creating |script_string|?

  MaybeLocal<String> script_string =
      String::NewFromUtf8(isolate_, script_source.source.c_str(), NewStringType::kNormal);
  MaybeLocal<String> filename_string =
      String::NewFromUtf8(isolate_, script_source.filename.c_str(), NewStringType::kNormal);

  if (script_string.IsEmpty() || filename_string.IsEmpty())
    return false;

  ScriptOrigin origin(filename_string.ToLocalChecked());
  Local<String> source = script_string.ToLocalChecked();

  TryCatch try_catch;

  MaybeLocal<Script> script = Script::Compile(context(), source, &origin);
  if (script.IsEmpty()) {
    // The script could not be compiled - syntax error?
    DispatchException(try_catch);
    return false;
  }

  MaybeLocal<Value> script_result = script.ToLocalChecked()->Run(context());
  if (try_catch.HasCaught()) {
    DispatchException(try_catch);
    return false;
  }

  if (result)
    *result = handle_scope.Escape(script_result.ToLocalChecked());

  return true;
}

bool Runtime::Call(v8::Local<v8::Function> function, size_t argument_count, v8::Local<v8::Value> arguments[]) {
  TryCatch try_catch;

  Local<Context> current_context = context();
  MaybeLocal<Value> result = function->Call(current_context,
                                            current_context->Global(), argument_count, arguments);

  if (result.IsEmpty() || try_catch.HasCaught()) {
    DispatchException(try_catch);
    return false;
  }

  return true;
}

void Runtime::DispatchException(const TryCatch& try_catch) {
  if (!delegate_)
    return;

  // TODO: Exceptions ideally should have stack traces attached to them.

  Local<Message> message = try_catch.Message();

  // Extract the exception message from the try-catch block.
  String::Utf8Value exception(try_catch.Exception());
  std::string exception_string(*exception, exception.length());

  // Extract the filename of the script from the try-catch block.
  String::Utf8Value resource_name(message->GetScriptOrigin().ResourceName());
  std::string filename(*resource_name, resource_name.length());

  size_t line_number = message->GetLineNumber();

  delegate_->OnScriptError(filename, line_number, exception_string);
}

}  // namespace bindings
