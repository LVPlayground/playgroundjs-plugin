// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/global_scope.h"

#include "base/logging.h"
#include "bindings/objects/console.h"
#include "bindings/objects/event.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

GlobalScope* g_global_scope = nullptr;

void AddEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  g_global_scope->addEventListener(arguments);
}

void RemoveEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  g_global_scope->removeEventListener(arguments);
}

}  // namespace

GlobalScope::GlobalScope(Runtime* runtime)
    : runtime_(runtime),
      console_(new Console(runtime)) {
  g_global_scope = this;
}

GlobalScope::~GlobalScope() {
  g_global_scope = nullptr;
}

void GlobalScope::CreateInterfaceForCallback(const plugin::Callback& callback) {
  events_[callback.name] = std::make_unique<Event>(runtime_, callback);
}

v8::Local<v8::Object> GlobalScope::CreateEventForCallback(const plugin::Callback& callback,
                                                          const plugin::Arguments& arguments) {
  auto event_iter = events_.find(callback.name);
  if (event_iter == events_.end())
    return v8::Local<v8::Object>();

  return event_iter->second->CreateInstance(arguments);
}

// addEventListener([event_name], [function])
void GlobalScope::addEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  v8::Isolate* isolate = runtime_->isolate();

  if (arguments.Length() < 2 || !arguments[0]->IsString() || !arguments[1]->IsFunction()) {
    LOG(ERROR) << "addEventListener() requires two parameters: [event_name] and [function].";
    // TODO: Throw an exception instead of outputting to the console.
    return;
  }

  const std::string event_name = toString(v8::Local<v8::String>::Cast(arguments[0]));
  event_listeners_[event_name].push_back(
      v8::Persistent<v8::Function>(isolate, v8::Local<v8::Function>::Cast(arguments[1])));
}

// removeEventListener([event_name][, [function]]?)
void GlobalScope::removeEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  v8::Isolate* isolate = runtime_->isolate();

  if (arguments.Length() < 1 || !arguments[0]->IsString()) {
    LOG(ERROR) << "removeEventListener() requires one parameter: [event_name] (and optionally [function]).";
    // TODO: Throw an exception instead of outputting to the console.
    return;
  }

  const std::string event_name = toString(v8::Local<v8::String>::Cast(arguments[0]));

  auto event_list_iter = event_listeners_.find(event_name);
  if (event_list_iter == event_listeners_.end())
    return;

  if (arguments.Length() > 1) {
    v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(arguments[1]);

    auto event_iter = event_list_iter->second.begin();
    while (event_iter != event_list_iter->second.end()) {
      if (*event_iter == function)
        event_iter = event_list_iter->second.erase(event_iter);
      else
        ++event_iter;
    }
  } else {
    event_list_iter->second.clear();
  }
}

bool GlobalScope::triggerEvent(const std::string& event_name, v8::Local<v8::Object> object) {
  auto event_list_iter = event_listeners_.find(event_name);
  if (event_list_iter == event_listeners_.end())
    return false;

  v8::Isolate* isolate = runtime_->isolate();

  for (const auto& persistent_function : event_list_iter->second) {
    v8::Local<v8::Function> function = v8::Local<v8::Function>::New(isolate, persistent_function);
    
    v8::Local<v8::Value> arguments[1];
    arguments[0] = object;

    runtime_->Call(function, 1u, arguments);
  }

  return Event::HasDefaultBeenPrevented(object);
}

void GlobalScope::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = runtime_->isolate();

  global->Set(v8String("addEventListener"),
              v8::FunctionTemplate::New(isolate, AddEventListenerCallback));
  global->Set(v8String("removeEventListener"),
              v8::FunctionTemplate::New(isolate, RemoveEventListenerCallback));

  console_->InstallPrototype(global);
  for (const auto& pair : events_)
    pair.second->InstallPrototype(global);
}

void GlobalScope::InstallObjects(v8::Local<v8::Object> global) {
  // Install the "self" object, which refers to the global scope directly.
  global->Set(v8String("self"), global);

  console_->InstallObjects(global);
  for (const auto& pair : events_)
    pair.second->InstallObjects(global);
}

void GlobalScope::Dispose() {
  event_listeners_.clear();
}

}  // namespace bindings
