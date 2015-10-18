// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql_module.h"

#include "playground/bindings/utilities.h"

namespace bindings {

namespace {

// void MySQL.prototype.close();
void MySQLCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {}

// number MySQL.prototype.query(string query);
void MySQLQueryCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {}

}  // namespace

MySQLModule::MySQLModule() {}

MySQLModule::~MySQLModule() {}

void MySQLModule::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate);

  v8::Local<v8::Template> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("close"), v8::FunctionTemplate::New(isolate, MySQLCloseCallback));
  prototype_template->Set(v8String("query"), v8::FunctionTemplate::New(isolate, MySQLQueryCallback));

  global->Set(v8String("MySQL"), function_template);
}

}  // namespace bindings
