// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/console.h"

#include <set>
#include <sstream>

#include "base/logging.h"
#include "bindings/global_scope.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

#include <include/v8.h>

namespace bindings {

namespace {

// Indentation size for array and object printing.
const size_t kIndentSize = 4;

// TODO: Proper detection of recursive calls.
std::string ValueToString(v8::Local<v8::Value> value, size_t indent) {
  DCHECK(!value.IsEmpty());
  if (value->IsNull()) {
    return "[NULL]";

  } else if (value->IsArray()) {
    std::stringstream output;
    output << "[\n";
    
    v8::Local<v8::Array> array_value = v8::Local<v8::Array>::Cast(value);
    DCHECK(!array_value.IsEmpty());

    const size_t new_indent = indent + kIndentSize;

    const std::string prefix(indent, ' ');
    const std::string new_prefix(new_indent, ' ');

    for (size_t index = 0; index < array_value->Length(); ++index)
      output << new_prefix << ValueToString(array_value->Get(index), new_indent) << ",\n";

    output << prefix << "]";

    return output.str();

  } else if (value->IsObject()) {
    std::stringstream output;
    output << "{\n";
    
    v8::Local<v8::Object> object_value = v8::Local<v8::Object>::Cast(value);
    DCHECK(!object_value.IsEmpty());

    const size_t new_indent = indent + kIndentSize;

    const std::string prefix(indent, ' ');
    const std::string new_prefix(new_indent, ' ');

    v8::Local<v8::Array> local_properties = object_value->GetOwnPropertyNames();
    for (size_t index = 0; index < local_properties->Length(); ++index) {
      v8::Local<v8::Value> member_key = local_properties->Get(index);
      v8::Local<v8::Value> member_value = object_value->Get(member_key);

      if (member_value->StrictEquals(value) || !member_key->IsString())
        continue;

      output << new_prefix << ValueToString(member_key, 0) << " => ";
      output << ValueToString(member_value, new_indent) << ",\n";
    }

    output << prefix << "}";

    return output.str();
  }

  v8::String::Utf8Value string(value);
  if (*string == nullptr)
    return "[unknown]";

  if (value->IsString())
    return "\"" + std::string(*string, string.length()) + "\"";
  
  return std::string(*string, string.length());
}

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
  v8::Local<v8::Value> function_value = global->Get(v8String("Console"));
  DCHECK(function_value->IsFunction());

  v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(function_value);

  global->Set(v8String("console"), function->NewInstance());
}

void Console::OutputValue(v8::Local<v8::Value> value) const {
  std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
  if (!runtime->delegate())
    return;

  // Fast-path for string-only values, which won't get surrounded in quotes this way.
  if (value->IsString())
    runtime->delegate()->OnScriptOutput(toString(value));
  else
    runtime->delegate()->OnScriptOutput(ValueToString(value, 0 /** indent **/));
}

}  // namespace
