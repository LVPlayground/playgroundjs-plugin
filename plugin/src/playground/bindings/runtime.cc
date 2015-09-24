// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string.h>
#include <unordered_map>

#include <include/libplatform/libplatform.h>

#include "base/logging.h"
#include "base/time.h"
#include "bindings/allocator.h"
#include "bindings/exception_handler.h"
#include "bindings/global_scope.h"
#include "bindings/script_prologue.h"
#include "bindings/utilities.h"

using namespace v8;

namespace bindings {

namespace {

// Map of v8 Isolates to the associated Runtime instances (weak references).
std::unordered_map<v8::Isolate*, std::weak_ptr<Runtime>> g_runtime_instances_;

// String of the v8 flags that should apply to this virtual machine. See flag-definitions.h in
// the v8 source code for a list of all supported command line flags.
const char kRuntimeFlags[] =
    "--use_strict "
    "--harmony "
    "--harmony_destructuring "
    "--harmony_default_parameters";

// Returns whether |character| represents a line break.
bool IsLineBreak(char character) {
  return character == '\n' || character == '\r';
}

// Returns a string with all line breaks removed from |line|.
std::string RemoveLineBreaks(std::string line) {
  line.erase(std::remove_if(line.begin(), line.end(), IsLineBreak), line.end());
  return line;
}

// Callback for the v8 message handler. Forward the call to the exception handler.
void MessageCallback(v8::Local<v8::Message> message, v8::Local<v8::Value> error) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (runtime)
    runtime->GetExceptionHandler()->OnMessage(message, error);
}

// Callback for the v8 fatal error handler. Forward the call to the exception handler.
void FatalErrorCallback(const char* location, const char* message) {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (runtime)
    runtime->GetExceptionHandler()->OnFatalError(location, message);
}

}  // namespace

// static
std::shared_ptr<Runtime> Runtime::FromIsolate(v8::Isolate* isolate) {
  const auto instance = g_runtime_instances_.find(isolate);
  if (instance == g_runtime_instances_.end())
    return nullptr;

  CHECK(!instance->second.expired());
  return instance->second.lock();
}

// static
std::shared_ptr<Runtime> Runtime::Create(Delegate* runtime_delegate,
                                         plugin::PluginController* plugin_controller) {
  auto instance = std::shared_ptr<Runtime>(
      new Runtime(runtime_delegate, plugin_controller));
  CHECK(instance->isolate());

  g_runtime_instances_[instance->isolate()] = instance;
  return instance;
}

Runtime::Runtime(Delegate* runtime_delegate,
                 plugin::PluginController* plugin_controller)
    : global_scope_(new GlobalScope(plugin_controller)),
      runtime_delegate_(runtime_delegate) {
  V8::InitializeICU();

  platform_.reset(platform::CreateDefaultPlatform());
  V8::InitializePlatform(platform_.get());

  V8::SetFlagsFromString(kRuntimeFlags, sizeof(kRuntimeFlags));
  V8::Initialize();

  allocator_.reset(new SimpleArrayBufferAllocator());

  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();

  isolate_ = Isolate::New(create_params);
  isolate_scope_.reset(new Isolate::Scope(isolate_));

  exception_handler_.reset(new ExceptionHandler(runtime_delegate_));
  isolate_->SetCaptureStackTraceForUncaughtExceptions(true, 15);
  isolate_->AddMessageListener(MessageCallback);
  isolate_->SetFatalErrorHandler(FatalErrorCallback);

  // TODO: This should be set by some sort of Configuration object.
  script_directory_ = base::FilePath("javascript");
}

Runtime::~Runtime() {
  g_runtime_instances_.erase(isolate_);

  global_scope_.reset();

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
  ScriptSource global_prologue(kScriptPrologue);
  if (!Execute(global_prologue, nullptr /** result **/))
    LOG(ERROR) << "Unable to install the global script prologue in the virtual machine.";
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

  script.source = source_stream.str();

  // Now execute |script| normally on the runtime.
  return Execute(script, result);
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
