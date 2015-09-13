// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/objects/event.h"

#include "base/logging.h"
#include "base/string_piece.h"
#include "plugin/arguments.h"

namespace bindings {

namespace {

// Index in the instance object's aligned pointer fields for the preventDefault flag.
int kInternalEventPreventDefaultedIndex = 0;

// Converts a Pawn callback name to a JavaScript event name. Some examples:
//     OnGameModeInit => GameModeInitEvent
//     OnPlayerConnect => PlayerConnectEvent
std::string ToJavaScriptEventName(base::StringPiece pawn_callback_name) {
  if (!pawn_callback_name.starts_with("On"))
    return pawn_callback_name.as_string();

  pawn_callback_name.remove_prefix(2);

  return pawn_callback_name.as_string() + "Event";
}

}  // namespace

Event::Event(Runtime* runtime, const plugin::Callback& callback) 
    : GlobalObject(runtime),
      callback_(callback) {
  interface_name_ = ToJavaScriptEventName(callback.name);
}

Event::~Event() {}

v8::Local<v8::Object> Event::CreateInstance(const plugin::Arguments& arguments) {
  v8::Local<v8::Value> function_value = context()->Global()->Get(v8String(interface_name_));
  DCHECK(function_value->IsFunction());

  v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(function_value);
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::Object> instance = function->NewInstance();
  for (const auto& argument : callback_.arguments) {
    v8::Local<v8::String> property = v8String(argument.first);
    switch (argument.second) {
    case plugin::ARGUMENT_TYPE_INT:
      instance->Set(property, v8::Number::New(isolate, arguments.GetInteger(argument.first)));
      break;

    case plugin::ARGUMENT_TYPE_FLOAT:
      instance->Set(property, v8::Number::New(isolate, arguments.GetFloat(argument.first)));
      break;

    case plugin::ARGUMENT_TYPE_STRING:
      instance->Set(property, v8String(arguments.GetString(argument.first)));
      break;
    }
  }

  return instance;
}

void Event::InstallPrototype(v8::Local<v8::ObjectTemplate> global) {
  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate());
  v8::Local<v8::ObjectTemplate> object_template = function_template->InstanceTemplate();

  // TODO: Special-case "playerid", "vehicleid" and so on.
  for (const auto& pair : callback_.arguments)
    object_template->Set(v8String(pair.first), v8::Undefined(isolate()));

  if (callback_.cancelable) {
    v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

    // Make sure that there is a location to store the preventDefaulted flag.
    object_template->SetInternalFieldCount(1 /** kInternalEventPreventDefaultedIndex **/);

    prototype_template->Set(v8String("preventDefault"),
                            v8::FunctionTemplate::New(isolate(), Event::PreventDefault));

    object_template->SetAccessorProperty(v8String("defaultPrevented"),
                                         v8::FunctionTemplate::New(isolate(), Event::DefaultPrevented),
                                         v8::Local<v8::FunctionTemplate>(),
                                         v8::PropertyAttribute::ReadOnly);
  }

  global->Set(v8String(interface_name_), function_template);

}

// static
bool Event::HasDefaultBeenPrevented(v8::Local<v8::Value> value) {
  if (value.IsEmpty() || !value->IsObject())
    return false;

  v8::Local<v8::Object> object_value = v8::Local<v8::Object>::Cast(value);
  if (!object_value->InternalFieldCount())
    return false;

  v8::Local<v8::Value> internal_flag = object_value->GetInternalField(kInternalEventPreventDefaultedIndex);
  if (internal_flag.IsEmpty() || !internal_flag->IsBoolean())
    return false;

  return internal_flag->BooleanValue();
}

// static
void Event::PreventDefault(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (arguments.Holder().IsEmpty() || !arguments.Holder()->IsObject())
    return;


  v8::Local<v8::Object> object_value = v8::Local<v8::Object>::Cast(arguments.Holder());
  if (!object_value->InternalFieldCount())
    return;

  object_value->SetInternalField(kInternalEventPreventDefaultedIndex,
                                 v8::Boolean::New(v8::Isolate::GetCurrent(), true));
}

// static
void Event::DefaultPrevented(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  arguments.GetReturnValue().Set(HasDefaultBeenPrevented(arguments.Holder()));
}

}  // namespace bindings
