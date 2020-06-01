// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/event.h"

#include <algorithm>
#include <unordered_map>

#include "base/encoding.h"
#include "base/logging.h"
#include "base/string_piece.h"
#include "bindings/utilities.h"
#include "plugin/arguments.h"

#include <include/v8.h>

namespace bindings {

namespace {

// Index in the instance object's internal fields for the DefaultPrevented flag.
int kInternalEventDefaultPreventedIndex = 0;

// Map from SA-MP callback name to idiomatic JavaScript event type.
std::unordered_map<std::string, std::string> g_callback_type_map_;

// Converts |callback| to an idiomatic JavaScript event type. This means we remove the "On"
// prefix and lowercase the rest of the event, e.g. OnPlayerConnect -> playerconnect.
std::string CreateEventType(base::StringPiece callback) {
  if (callback.starts_with("On"))
    callback.remove_prefix(2 /** strlen("On") **/);

  std::string event_type = callback.as_string();
  std::transform(event_type.begin(), event_type.end(), event_type.begin(), ::tolower);

  return event_type;
}

// Converts |callback| to an idiomatic JavaScript interface name. This means we remove the "On"
// prefix and append "Event", e.g. OnPlayerConnect -> PlayerConnectEvent.
std::string CreateEventInterfaceName(base::StringPiece callback) {
  if (callback.starts_with("On"))
    callback.remove_prefix(2 /** strlen("On") **/);

  return callback.as_string() + "Event";
}

// Updates the internal DefaultPrevented flag for the holder to |true|. Note that this method
// does not have very sophisticated checks to prevent it from being bound to other internal
// objects, so let's hope people don't play weird tricks.
void PreventDefaultCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (arguments.Holder().IsEmpty() || !arguments.Holder()->IsObject())
    return;

  v8::Local<v8::Object> object_value = v8::Local<v8::Object>::Cast(arguments.Holder());
  if (!object_value->InternalFieldCount())
    return;

  object_value->SetInternalField(kInternalEventDefaultPreventedIndex,
                                 v8::True(v8::Isolate::GetCurrent()));
}

// Returns whether the default action of the holder has been prevented.
void DefaultPreventedCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  arguments.GetReturnValue().Set(Event::DefaultPrevented(arguments.Holder()));
}

}  // namespace

// static
std::unique_ptr<Event> Event::Create(const plugin::Callback& callback) {
  return std::unique_ptr<Event>(new Event(callback));
}

// static
bool Event::DefaultPrevented(v8::Local<v8::Value> value) {
  if (value.IsEmpty() || !value->IsObject())
    return false;

  v8::Local<v8::Object> object_value = v8::Local<v8::Object>::Cast(value);
  if (!object_value->InternalFieldCount())
    return false;

  v8::Local<v8::Value> internal_flag = object_value->GetInternalField(kInternalEventDefaultPreventedIndex);
  if (internal_flag.IsEmpty() || !internal_flag->IsBoolean())
    return false;

  return internal_flag->BooleanValue(GetIsolate());
}

// static
const std::string& Event::ToEventType(const std::string& callback) {
  if (!g_callback_type_map_.count(callback))
    g_callback_type_map_[callback] = CreateEventType(callback);

  return g_callback_type_map_[callback];
}

Event::Event(const plugin::Callback& callback) 
    : callback_(callback),
      event_type_(CreateEventInterfaceName(callback.name)) {}

Event::~Event() {}

void Event::InstallPrototype(v8::Local<v8::ObjectTemplate> global) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate);
  v8::Local<v8::ObjectTemplate> object_template = function_template->InstanceTemplate();

  // TODO: Special-case "playerid", "vehicleid" and so on.
  for (const auto& pair : callback_.arguments)
    object_template->Set(v8String(pair.first), v8::Undefined(isolate));

  // If the event is cancelable, we create the property "defaultPrevented" (a readonly boolean)
  // and the method preventDefault() to set it to true. This effect is irreversible.
  if (callback_.cancelable) {
    v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

    // Make sure that there is a location to store the DefaultPrevented flag.
    object_template->SetInternalFieldCount(1 /** kInternalEventDefaultPreventedIndex **/);

    prototype_template->Set(v8String("preventDefault"),
                            v8::FunctionTemplate::New(isolate, PreventDefaultCallback));

    object_template->SetAccessorProperty(
        v8String("defaultPrevented"),
        v8::FunctionTemplate::New(isolate, DefaultPreventedCallback),
        v8::Local<v8::FunctionTemplate>(),
        v8::PropertyAttribute::ReadOnly);
  }

  global->Set(v8String(event_type_), function_template);
}

v8::Local<v8::Object> Event::NewInstance(const plugin::Arguments& arguments) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Value> function_value = context->Global()->Get(context, v8String(event_type_)).ToLocalChecked();
  DCHECK(function_value->IsFunction());

  v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(function_value);
  v8::Local<v8::Object> instance = function->NewInstance(context).ToLocalChecked();

  for (const auto& argument : callback_.arguments) {
    v8::Local<v8::String> property = v8String(argument.first);
    switch (argument.second) {
    case plugin::ARGUMENT_TYPE_INT:
      instance->Set(context, property, v8::Number::New(isolate, arguments.GetInteger(argument.first)));
      break;

    case plugin::ARGUMENT_TYPE_FLOAT:
      instance->Set(context, property, v8::Number::New(isolate, arguments.GetFloat(argument.first)));
      break;

    case plugin::ARGUMENT_TYPE_STRING:
      {
        const std::string before = arguments.GetString(argument.first);
        const std::string after = fromAnsi(before);

        LOG(INFO) << "Before: [" << before << "]";
        LOG(INFO) << "After:  [" << after << "]";

        instance->Set(context, property, v8String(after));
      }
      break;
    }
  }

  return instance;
}

}  // namespace bindings
