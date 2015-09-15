// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/global_scope_event_target.h"

#include "base/logging.h"
#include "bindings/objects/event.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

GlobalScopeEventTarget* g_global_scope = nullptr;

}  // namespace

void AddEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (g_global_scope)
    g_global_scope->addEventListener(arguments);
}

void RemoveEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (g_global_scope)
    g_global_scope->removeEventListener(arguments);
}

GlobalScopeEventTarget::GlobalScopeEventTarget(Runtime* runtime) : runtime_(runtime) {
  g_global_scope = this;
}

GlobalScopeEventTarget::~GlobalScopeEventTarget() {
  g_global_scope = nullptr;
}

// addEventListener([event_name], [function])
void GlobalScopeEventTarget::addEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  v8::Isolate* isolate = runtime_->isolate();

  if (arguments.Length() < 2 || !arguments[0]->IsString() || !arguments[1]->IsFunction()) {
    ThrowException("unable to execute addEventListener(): 2 argument required, but only " +
                       std::to_string(arguments.Length()) + " present");
    return;
  }

  const std::string event_name = toString(v8::Local<v8::String>::Cast(arguments[0]));
  event_listeners_[event_name].push_back(
      v8::Persistent<v8::Function>(isolate, v8::Local<v8::Function>::Cast(arguments[1])));
}

// removeEventListener([event_name][, [function]]?)
void GlobalScopeEventTarget::removeEventListener(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  v8::Isolate* isolate = runtime_->isolate();

  if (arguments.Length() < 1 || !arguments[0]->IsString()) {
    ThrowException("unable to execute addEventListener(): 2 argument required, but only 0 present");
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
  }
  else {
    event_list_iter->second.clear();
  }
}

bool GlobalScopeEventTarget::triggerEvent(const std::string& event_name, v8::Local<v8::Object> object) {
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

}  // namespace bindings
