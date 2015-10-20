// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/mysql_module.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "base/logging.h"
#include "base/macros.h"
#include "playground/bindings/frame_observer.h"
#include "playground/bindings/modules/mysql/connection_delegate.h"
#include "playground/bindings/modules/mysql/connection_host.h"
#include "playground/bindings/modules/mysql/result_entry.h"
#include "playground/bindings/promise.h"
#include "playground/bindings/utilities.h"

namespace bindings {

namespace {

// The MySQL class encapsulates a single connection to a MySQL server with a given set of
// information. The connection may be used for any number of queries, and will continue to try
// to auto-reconnect when issues with the connection are identified.
class MySQL : public mysql::ConnectionDelegate,
              public FrameObserver {
 public:
  MySQL(const std::string& hostname, const std::string& username, const std::string& password, const std::string& database, int port)
      : frame_observer_(this),
        hostname_(hostname),
        username_(username),
        password_(password),
        database_(database),
        port_(port) {
    // Initialize the |ready| promise, which will be informed of the connection result.
    ready_.reset(new Promise());

    // Begin establishing the connection through the mysql::ConnectionHost.
    connection_.reset(new mysql::ConnectionHost(this));
    connection_->Connect(hostname, username, password, database, port);

    LOG(INFO) << "Connecting to " << hostname << ":" << std::to_string(port) << "...";
  }

  ~MySQL() override { close(); }

  // mysql::ConnectionDelegate implementation.
  void DidConnect(unsigned int request_id,
                  bool succeeded,
                  int error_number,
                  const std::string& error_message) override {
    DCHECK(!ready_->HasSettled());
    LOG(INFO) << "Connection to " << hostname_ << ":" << std::to_string(port_)
              << (succeeded ? " succeeded" : " failed") << ".";

    std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());

    v8::HandleScope handle_scope(runtime->isolate());
    v8::Context::Scope context_scope(runtime->context());

    // If the connection attempt succeeded, resolve the promise immediately.
    if (succeeded) {
      ready_->Resolve(true /* this should be NULL */);
      connected_ = true;
      return;
    }

    // Otherwise it failed. Output the reason to the console, and create an error object with
    // which we can reject the Promise, so that the JavaScript code knows as well.
    LOG(ERROR) << "MySQL (#" << error_number << "): " << error_message;

    v8::Local<v8::Object> error = v8::Object::New(runtime->isolate());
    error->Set(v8String("error"), v8::Integer::New(runtime->isolate(), error_number));
    error->Set(v8String("message"), v8::String::NewFromUtf8(runtime->isolate(), error_message.c_str()));

    ready_->Reject(error);

    // Close the connection. We require the first connection attempt to succeed.
    connection_->Close();
  }

  void DidQuery(unsigned int request_id, std::shared_ptr<mysql::ResultEntry> result) override {
    if (!queries_.count(request_id)) {
      LOG(ERROR) << "Received an unexpected response for request " << request_id;
      return;
    }

    std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
    {
      v8::HandleScope handle_scope(runtime->isolate());
      v8::Context::Scope context_scope(runtime->context());

      // TODO(Russell): Convert |result| to a JavaScript object.
      queries_[request_id]->Resolve(true);
    }

    queries_.erase(request_id);
  }

  void DidQueryFail(unsigned int request_id, int error_number, const std::string& error_message) override {
    if (!queries_.count(request_id)) {
      LOG(ERROR) << "Received an unexpected response for request " << request_id;
      return;
    }

    std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(v8::Isolate::GetCurrent());
    {
      v8::HandleScope handle_scope(runtime->isolate());
      v8::Context::Scope context_scope(runtime->context());

      v8::Local<v8::Object> error = v8::Object::New(runtime->isolate());
      error->Set(v8String("error"), v8::Integer::New(runtime->isolate(), error_number));
      error->Set(v8String("message"), v8::String::NewFromUtf8(runtime->isolate(), error_message.c_str()));

      queries_[request_id]->Reject(error);
    }

    queries_.erase(request_id);
  }

  // bindings::FrameObserver implementation.
  void OnFrame() override {
    connection_->ProcessUpdates();
  }

  // Executes |query| on this connection. The returned promise will be resolved when the result
  // is known, or rejected when there was a problem when executing the query.
  v8::Local<v8::Promise> query(const std::string& query) {
    ++total_query_count_;

    unsigned int request_id = connection_->Query(query);

    std::shared_ptr<Promise> promise = std::make_shared<Promise>();
    queries_[request_id] = promise;

    return promise->GetPromise();
  }

  // Immediately closes the connection with the MySQL database.
  void close() {
    LOG(INFO) << "Closing the connection to " << hostname_ << ":" << std::to_string(port_) << "...";
    connection_->Close();
  }

  // A promise that will be settled when it's known whether the connection could be established
  // with the database. This will always return the same promise.
  v8::Local<v8::Promise> ready() const { return ready_->GetPromise(); }

  // Returns whether this MySQL instance has successfully connected to a database.
  bool connected() const { return connected_; }

  // Number of queries that have been executed, regardless of whether they succeeded.
  int total_query_count() const { return total_query_count_; }

  // Number of queries which are still in-progress, and no result is known of.
  int unresolved_query_count() const { return queries_.size(); }

  // Returns the information with which the MySQL connection was established.
  v8::Local<v8::String> hostname() const { return v8String(hostname_); }
  v8::Local<v8::String> username() const { return v8String(username_); }
  v8::Local<v8::String> password() const { return v8String(password_); }
  v8::Local<v8::String> database() const { return v8String(database_); }
  int port() const { return port_; }

  // Installs a weak reference to |object|, which is the JavaScript object that owns this instance.
  // A callback will be used to determine when it has been collected, so we can free up resources.
  void WeakBind(v8::Isolate* isolate, v8::Local<v8::Object> object) {
    object_.Reset(isolate, object);
    object_.SetWeak(this, OnGarbageCollected);
  }

 private:
  // Called when a MySQL instance has been garbage collected by the v8 engine. While this mechanism
  // for keeping track of collections is not guaranteed to work, it works now and it solves the
  // lifetime issues we were otherwise facing.
  static void OnGarbageCollected(const v8::WeakCallbackData<v8::Object, MySQL>& data) {
    MySQL* instance = data.GetParameter();

    instance->object_.Reset();
    delete instance;
  }

  ScopedFrameObserver frame_observer_;

  std::string hostname_;
  std::string username_;
  std::string password_;
  std::string database_;
  int port_;

  std::unique_ptr<mysql::ConnectionHost> connection_;
  std::unordered_map<unsigned int, std::shared_ptr<Promise>> queries_;

  std::unique_ptr<Promise> ready_;

  bool connected_ = false;

  int total_query_count_ = 0;

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

  if (arguments.Length() < 5) {
    ThrowException("unable to construct MySQL: 5 argument required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[4]->IsNumber()) {
    ThrowException("unable to construct MySQL: expected an integer for the fifth argument.");
    return;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::MaybeLocal<v8::Number> maybe_port = arguments[4]->ToNumber(isolate->GetCallingContext());
  if (maybe_port.IsEmpty())
    return;  // ???

  v8::Local<v8::Number> port = maybe_port.ToLocalChecked();

  MySQL* instance = new MySQL(toString(arguments[0]),  // hostname
                              toString(arguments[1]),  // username
                              toString(arguments[2]),  // password
                              toString(arguments[3]),  // database
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

// -------------------------------------------------------------------------------------------------

#define DEFINE_MAPPING(Name, Member) \
  void Name(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) { \
    MySQL* instance = GetInstanceFromObject(info.This()); \
    if (!instance) return; \
    info.GetReturnValue().Set(instance->Member()); \
  }

DEFINE_MAPPING(MySQLReadyGetter, ready);

DEFINE_MAPPING(MySQLConnectedGetter, connected);

DEFINE_MAPPING(MySQLTotalQueryCountGetter, total_query_count);
DEFINE_MAPPING(MySQLUnresolvedQueryCountGetter, unresolved_query_count);

DEFINE_MAPPING(MySQLHostnameGetter, hostname);
DEFINE_MAPPING(MySQLUsernameGetter, username);
DEFINE_MAPPING(MySQLPasswordGetter, password);
DEFINE_MAPPING(MySQLDatabaseGetter, database);
DEFINE_MAPPING(MySQLPortGetter, port);

#undef DEFINE_MAPPING

// -------------------------------------------------------------------------------------------------

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

  prototype_template->SetAccessor(v8String("hostname"), MySQLHostnameGetter);
  prototype_template->SetAccessor(v8String("username"), MySQLUsernameGetter);
  prototype_template->SetAccessor(v8String("password"), MySQLPasswordGetter);
  prototype_template->SetAccessor(v8String("database"), MySQLDatabaseGetter);
  prototype_template->SetAccessor(v8String("port"), MySQLPortGetter);

  global->Set(v8String("MySQL"), function_template);
}

}  // namespace bindings
