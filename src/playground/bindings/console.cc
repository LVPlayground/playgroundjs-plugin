// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/console.h"

#include <set>
#include <sstream>

#include "base/logging.h"
#include "bindings/exception_handler.h"
#include "bindings/global_scope.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

#include <include/v8.h>

namespace bindings {

namespace {

class ValueStringBuilder {
 public:
  ValueStringBuilder() {}
  ~ValueStringBuilder() {}

  const size_t kIndentStep = 2;

  void Write(v8::Local<v8::Value> value, size_t indent = 0) {
    DCHECK(!value.IsEmpty());

    if (value->IsNull())
      WriteNull();
    else if (value->IsNativeError())
      WriteError(value);
    else if (value->IsArray())
      WriteArray(value, indent);
    else if (value->IsObject())
      WriteObject(value, indent);
    else
      WriteGeneric(value);
  }

  void WriteNull() {
    stream_ << "[NULL]";
  }

  void WriteError(v8::Local<v8::Value> value) {
    v8::Local<v8::Message> message = v8::Exception::CreateMessage(v8::Isolate::GetCurrent(), value);
    if (message.IsEmpty())
      return;

    Runtime::FromIsolate(v8::Isolate::GetCurrent())->GetExceptionHandler()->OnMessage(
        message, value, ExceptionHandler::MessageSource::kScript);
  }

  void WriteArray(v8::Local<v8::Value> value, size_t indent) {
    v8::Local<v8::Array> array_value = v8::Local<v8::Array>::Cast(value);
    auto context = GetContext();

    const std::string prefix(indent, ' ');
    const std::string new_prefix(indent + kIndentStep, ' ');

    stream_ << "[\n";
    for (size_t index = 0; index < array_value->Length(); ++index) {
      stream_ << new_prefix;
      Write(array_value->Get(context, indent + kIndentStep).ToLocalChecked());
      stream_ << ",\n";
    }

    stream_ << prefix << "]";
  }

  void WriteObject(v8::Local<v8::Value> value, size_t indent) {
    v8::Local<v8::Object> object_value = v8::Local<v8::Object>::Cast(value);
    auto context = GetContext();

    const std::string prefix(indent, ' ');
    const std::string new_prefix(indent + kIndentStep, ' ');

    stream_ << "{\n";

    v8::Local<v8::Array> properties = object_value->GetOwnPropertyNames(context).ToLocalChecked();
    for (size_t index = 0; index < properties->Length(); ++index) {
      v8::Local<v8::Value> member_key = properties->Get(context, index).ToLocalChecked();
      v8::Local<v8::Value> member_value = object_value->Get(context, member_key).ToLocalChecked();

      if (member_value->StrictEquals(value) || !member_key->IsString())
        continue;  // TODO: Implement printing of non-string values. Symbols?

      stream_ << new_prefix;
      Write(member_key, 0);
      stream_ << " => ";
      Write(member_value, indent + kIndentStep);
      stream_ << ",\n";
    }

    stream_ << prefix << "}";
  }

  void WriteGeneric(v8::Local<v8::Value> value) {
    v8::String::Utf8Value string(GetIsolate(), value);
    if (*string == nullptr) {
      stream_ << "[unknown]";
      return;
    }

    if (value->IsString()) stream_ << "\"";
    stream_ << std::string(*string, string.length());
    if (value->IsString()) stream_ << "\"";
  }

  std::string str() { return stream_.str(); }

 private:
  std::stringstream stream_;
};

// Iterates over all callbacks passed to the console.log() function and forward the call to the
// Console instance, where creation and output of the representations will be handled.
void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  GlobalScope* global = Runtime::FromIsolate(arguments.GetIsolate())->GetGlobalScope();

  for (int index = 0; index < arguments.Length(); ++index)
    global->GetConsole()->OutputValue(arguments[index]);
}

}  // namespace

void Console::InstallPrototype(v8::Local<v8::ObjectTemplate> global) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate);

  v8::Local<v8::Template> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("log"), v8::FunctionTemplate::New(isolate, ConsoleLogCallback));

  global->Set(v8String("Console"), function_template);
}

void Console::InstallObjects(v8::Local<v8::Object> global) const {
  auto context = GetContext();

  v8::Local<v8::Value> function_value = global->Get(context, v8String("Console")).ToLocalChecked();
  DCHECK(function_value->IsFunction());

  v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(function_value);

  global->Set(context, v8String("console"), function->NewInstance(context).ToLocalChecked());
}

void Console::OutputValue(v8::Local<v8::Value> value) const {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (!runtime->delegate())
    return;

  // Fast-path for string-only values, which won't get surrounded in quotes this way.
  if (value->IsString()) {
    runtime->delegate()->OnScriptOutput(toString(value));
    return;
  }

  ValueStringBuilder builder;
  builder.Write(value);

  runtime->delegate()->OnScriptOutput(builder.str());
}

}  // namespace
