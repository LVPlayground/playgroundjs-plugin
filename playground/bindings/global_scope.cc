// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/global_scope.h"

#include "base/logging.h"
#include "bindings/objects/console.h"
#include "bindings/objects/event.h"
#include "bindings/objects/include.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace bindings {

GlobalScope::GlobalScope(Runtime* runtime)
    : GlobalScopeEventTarget(runtime),
      runtime_(runtime),
      console_(new Console(runtime)),
      include_(new Include(runtime)) {}

GlobalScope::~GlobalScope() {}

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

void GlobalScope::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = runtime_->isolate();

  global->Set(v8String("addEventListener"),
              v8::FunctionTemplate::New(isolate, AddEventListenerCallback));
  global->Set(v8String("removeEventListener"),
              v8::FunctionTemplate::New(isolate, RemoveEventListenerCallback));

  console_->InstallPrototype(global);
  include_->InstallPrototype(global);

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

}  // namespace bindings
