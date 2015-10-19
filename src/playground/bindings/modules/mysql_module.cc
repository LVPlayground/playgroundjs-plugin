// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql_module.h"

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "playground/bindings/promise.h"
#include "playground/bindings/utilities.h"

namespace bindings {

namespace {

// The MySQL class encapsulates a single connection to a MySQL server with a given set of
// information. The connection may be used for any number of queries, and will continue to try
// to auto-reconnect when issues with the connection are identified.
class MySQL {
 public:
  MySQL(const std::string& hostname, const std::string& username, const std::string& password, int port)
      : hostname_(hostname), username_(username), password_(password), port_(port) {
    // TODO(Russell): Begin establishing the connection.
  }

  ~MySQL() {}

  // Installs a weak reference to |object|, which is the JavaScript object that owns this instance.
  // A callback will be used to determine when it has been collected, so we can free up resources.
  void WeakBind(v8::Isolate* isolate, v8::Local<v8::Object> object) {
    object_.Reset(isolate, object);
    object_.SetWeak(this, OnGarbageCollected);
  }

  // Executes |query| on this connection. The returned promise will be resolved when the result
  // is known, or rejected when there was a problem when executing the query.
  v8::Local<v8::Promise> query(const std::string& query) { return v8::Local<v8::Promise>(); }

  // Immediately closes the connection with the MySQL database.
  void close() {}

  // A promise that will be settled when it's known whether the connection could be established
  // with the database. This will always return the same promise.
  v8::Local<v8::Promise> ready() const { return v8::Local<v8::Promise>(); }

  // Returns whether this MySQL instance has successfully connected to a database.
  bool connected() const { return connected_; }

  // Number of queries that have been executed, regardless of whether they succeeded.
  int total_query_count() const { return total_query_count_; }

  // Number of queries which are still in-progress, and no result is known of.
  int unresolved_query_count() const { return unresolved_query_count_; }

 private:
  // Called when a MySQL instance has been garbage collected by the v8 engine. While this mechanism
  // for keeping track of collections is not guaranteed to work, it works now and it solves the
  // lifetime issues we were otherwise facing.
  static void OnGarbageCollected(const v8::WeakCallbackData<v8::Object, MySQL>& data) {
    MySQL* instance = data.GetParameter();

    instance->object_.Reset();
    delete instance;
  }

  std::string hostname_;
  std::string username_;
  std::string password_;
  int port_;

  bool connected_ = false;

  int total_query_count_ = 0;
  int unresolved_query_count_ = 0;

  v8::Persistent<v8::Object> object_;

  DISALLOW_COPY_AND_ASSIGN(MySQL);
};

// -------------------------------------------------------------------------------------------------

MySQL* GetInstanceFromObject(v8::Local<v8::Object> object) {
  if (object.IsEmpty() || object->InternalFieldCount() != 1) {
    ThrowException("Expected a MySQL instance to be the |this| of the call.");
    return nullptr;
  }

  return static_cast<MySQL*>(object->GetAlignedPointerFromInternalField(0));
}

// MySQL.prototype.constructor(string hostname, string username, string password, int port)
void MySQLConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (!arguments.IsConstructCall()) {
    ThrowException("unable to construct MySQL: must only be used as a constructor.");
    return;
  }

  if (arguments.Length() < 4) {
    ThrowException("unable to construct MySQL: 4 argument required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[3]->IsNumber()) {
    ThrowException("unable to construct MySQL: expected an integer for the fourth argument.");
    return;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::MaybeLocal<v8::Number> maybe_port = arguments[3]->ToNumber(isolate->GetCallingContext());
  if (maybe_port.IsEmpty())
    return;  // ???

  v8::Local<v8::Number> port = maybe_port.ToLocalChecked();

  MySQL* instance = new MySQL(toString(arguments[0]),  // hostname
                              toString(arguments[1]),  // username
                              toString(arguments[2]),  // password
                              port->Int32Value());   // port

  instance->WeakBind(isolate, arguments.Holder());

  arguments.Holder()->SetAlignedPointerInInternalField(0, instance);
}

// Promise<sequence<object>> MySQL.prototype.query(string query);
void MySQLQueryCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  MySQL* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() == 0) {
    ThrowException("unable to execute MySQL.query(): 1 argument required, but only 0 provided.");
    return;
  }

  arguments.GetReturnValue().Set(instance->query(toString(arguments[0])));
}

// void MySQL.prototype.close();
void MySQLCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  MySQL* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  instance->close();
}

// Promise<> MySQL.prototype.ready
void MySQLReadyGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  MySQL* instance = GetInstanceFromObject(info.Holder());
  if (!instance)
    return;

  info.GetReturnValue().Set(instance->ready());
}

// bool MySQL.prototype.connected
void MySQLConnectedGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  MySQL* instance = GetInstanceFromObject(info.This());
  if (!instance)
    return;

  info.GetReturnValue().Set(instance->connected());
}

// int MySQL.prototype.totalQueryCount
void MySQLTotalQueryCountGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  MySQL* instance = GetInstanceFromObject(info.This());
  if (!instance)
    return;

  info.GetReturnValue().Set(instance->total_query_count());
}

// int MySQL.prototype.unresolvedQueryCount
void MySQLUnresolvedQueryCountGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  MySQL* instance = GetInstanceFromObject(info.This());
  if (!instance)
    return;

  info.GetReturnValue().Set(instance->unresolved_query_count());
}

}  // namespace

MySQLModule::MySQLModule() {}

MySQLModule::~MySQLModule() {}

void MySQLModule::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, MySQLConstructorCallback);

  v8::Local<v8::ObjectTemplate> instance_template = function_template->InstanceTemplate();
  instance_template->SetInternalFieldCount(1 /** for the native instance **/);

  v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("query"), v8::FunctionTemplate::New(isolate, MySQLQueryCallback));
  prototype_template->Set(v8String("close"), v8::FunctionTemplate::New(isolate, MySQLCloseCallback));

  prototype_template->SetAccessor(v8String("ready"), MySQLReadyGetter);

  prototype_template->SetAccessor(v8String("connected"), MySQLConnectedGetter);
  prototype_template->SetAccessor(v8String("totalQueryCount"), MySQLTotalQueryCountGetter);
  prototype_template->SetAccessor(v8String("unresolvedQueryCount"), MySQLUnresolvedQueryCountGetter);

  global->Set(v8String("MySQL"), function_template);
}

}  // namespace bindings
