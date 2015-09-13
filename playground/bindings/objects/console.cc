// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/objects/console.h"

#include "base/logging.h"

#include <set>
#include <sstream>

namespace bindings {

namespace {

// Indentation size for array and object printing.
const size_t kIndentSize = 4;

size_t in = 0;

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

}  // namespace

// Index in the instance object's aligned pointer fields for the native Console instance.
int kInternalConsoleInstanceIndex = 0;

// Name of the interface to be exposed on the runtime's global.
const char kInterfaceName[] = "Console";

// Name of the instance of the interface which is to be constructed on the runtime's global.
const char kInstanceName[] = "console";

Console::Console(Runtime* runtime) 
    : GlobalObject(runtime) {}

Console::~Console() {}

void Console::Output(Severity severity, const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (!arguments.Holder()->InternalFieldCount())
    return;  // called on the prototype object, rather than the instance.

  Console* instance = static_cast<Console*>(arguments.Holder()->GetAlignedPointerFromInternalField(kInternalConsoleInstanceIndex));
  for (int i = 0; i < arguments.Length(); ++i)
    instance->OutputArgument(severity, arguments[i]);
}

void Console::OutputArgument(Severity severity, v8::Local<v8::Value> value) {
  if (!runtime()->delegate())
    return;

  // TODO: Do we want to output |severity| somehow? It kind of pollutes the output.

  runtime()->delegate()->OnScriptOutput(ValueToString(value, 0 /* indent */));
}

void Console::InstallPrototype(v8::Local<v8::ObjectTemplate> global) {
  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate());
  function_template->InstanceTemplate()->SetInternalFieldCount(1 /** to store the instance **/);

  v8::Local<v8::Template> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("error"), v8::FunctionTemplate::New(isolate(), Console::Error));
  prototype_template->Set(v8String("info"), v8::FunctionTemplate::New(isolate(), Console::Info));
  prototype_template->Set(v8String("log"), v8::FunctionTemplate::New(isolate(), Console::Log));
  prototype_template->Set(v8String("warn"), v8::FunctionTemplate::New(isolate(), Console::Warn));

  global->Set(v8String(kInterfaceName), function_template);
}

void Console::InstallObjects(v8::Local<v8::Object> global) {
  v8::Local<v8::Value> function_value = global->Get(v8String(kInterfaceName));
  DCHECK(function_value->IsFunction());

  v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(function_value);

  v8::Local<v8::Object> instance = function->NewInstance();
  instance->SetAlignedPointerInInternalField(kInternalConsoleInstanceIndex, this);

  global->Set(v8String(kInstanceName), instance);
}

}  // namespace
