// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <include/libplatform/libplatform.h>

#include "base/logging.h"
#include "base/time.h"
#include "bindings/allocator.h"
#include "bindings/global_scope.h"
#include "bindings/runtime_options.h"
#include "bindings/script_prologue.h"
#include "bindings/utilities.h"

using namespace v8;

namespace bindings {

namespace {

// Name of the event on the global scope to invoke every frame.
const char kFrameEventName[] = "frame";

// Returns whether |character| represents a line break.
bool IsLineBreak(char character) {
  return character == '\n' || character == '\r';
}

// Returns a string with all line breaks removed from |line|.
std::string RemoveLineBreaks(std::string line) {
  line.erase(std::remove_if(line.begin(), line.end(), IsLineBreak), line.end());
  return line;
}

}  // namespace

Runtime::Runtime(const RuntimeOptions& options,
                 const base::FilePath& script_directory,
                 Delegate* runtime_delegate)
    : script_directory_(script_directory),
      runtime_delegate_(runtime_delegate),
      frame_event_name_(kFrameEventName) {
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

  // Make sure that the global script prologue is loaded in the virtual machine.
  {
    ScriptSource global_prologue(kScriptPrologue);
    if (!Execute(global_prologue, nullptr /** result **/))
      LOG(ERROR) << "Unable to install the global script prologue in the virtual machine.";
  }
}

void Runtime::Dispose() {
  global_scope_.reset();
}

bool Runtime::Execute(const ScriptSource& script_source,
                      v8::Local<v8::Value>* result) {
  EscapableHandleScope handle_scope(isolate());
  Context::Scope context_scope(context());

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
    DisplayException(try_catch);
    return false;
  }

  MaybeLocal<Value> script_result = script.ToLocalChecked()->Run(context());
  if (try_catch.HasCaught()) {
    DisplayException(try_catch);
    return false;
  }

  if (result)
    *result = handle_scope.Escape(script_result.ToLocalChecked());

  return true;
}

bool Runtime::ExecuteFile(const base::FilePath& file,
                          ExecutionType execution_type,
                          v8::Local<v8::Value>* result) {
  const base::FilePath script_path = script_directory_.Append(file);

  std::ifstream handle(script_path.value().c_str());
  if (!handle.is_open() || handle.fail())
    return false;

  ScriptSource script;
  script.filename = file.value();

  std::stringstream source_stream;

  // Prepend and append the module prologue and epilogue if the execution type is set to module
  // style. Otherwise just pass through the file's source to |source_stream|.
  if (execution_type == EXECUTION_TYPE_MODULE)
    source_stream << RemoveLineBreaks(kModulePrologue);

  std::copy(std::istreambuf_iterator<char>(handle),
            std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(source_stream));
  
  if (execution_type == EXECUTION_TYPE_MODULE)
    source_stream << RemoveLineBreaks(kModuleEpilogue);

  script.source.swap(source_stream.str());

  // Now execute |script| normally on the runtime.
  return Execute(script, result);
}

void Runtime::OnFrame() {
  v8::HandleScope handle_scope(isolate());
  v8::Context::Scope context_scope(context());
  
  v8::Local<v8::Object> dict = v8::Object::New(isolate());
  dict->Set(v8String("now"), v8::Number::New(isolate(), base::monotonicallyIncreasingTime()));

  global_scope_->triggerEvent(frame_event_name_, dict);
}

bool Runtime::Call(v8::Local<v8::Function> function,
                   size_t argument_count,
                   v8::Local<v8::Value> arguments[]) {
  TryCatch try_catch;

  Local<Context> current_context = context();
  MaybeLocal<Value> result = function->Call(current_context, current_context->Global(),
                                            argument_count, arguments);

  if (result.IsEmpty() || try_catch.HasCaught()) {
    DisplayException(try_catch);
    return false;
  }

  return true;
}

void Runtime::DisplayException(const TryCatch& try_catch) {
  if (!runtime_delegate_)
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

  runtime_delegate_->OnScriptError(filename, line_number, exception_string);
}

}  // namespace bindings
