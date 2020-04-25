// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket_module.h"

#include <algorithm>

#include "base/logging.h"
#include "bindings/modules/socket/socket.h"
#include "bindings/modules/socket/socket_open_options.h"
#include "bindings/modules/socket/socket_protocol.h"
#include "bindings/modules/socket/tcp_socket.h"
#include "bindings/promise.h"
#include "bindings/runtime_operations.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

// Default timeout, in seconds, to wait for establishing a socket connection.
const int32_t kDefaultTimeoutSec = 30;

// Bindings class bridging lifetime between the JavaScript Socket object and the C++ one.
class SocketBindings : public socket::Socket::SocketObserver {
 public:
  enum class EventType {
    kClose,
    kError,
    kMessage,
  };

  explicit SocketBindings(std::unique_ptr<socket::BaseSocket> engine)
      : socket_(std::make_unique<socket::Socket>(std::move(engine), this)) {}

  ~SocketBindings() = default;

  // Returns a raw pointer to the Socket object owned by this binding.
  socket::Socket* socket() { return socket_.get(); }

  // Installs a weak reference to |object|, which is the JavaScript object that owns this instance.
  // A callback will be used to determine when it has been collected, so we can free up resources.
  void WeakBind(v8::Isolate* isolate, v8::Local<v8::Object> object) {
    object_.Reset(isolate, object);
    object_.SetWeak(this, OnGarbageCollected, v8::WeakCallbackType::kParameter);
  }

  // socket::Socket::SocketObserver implementation:
  void OnClose() override {
    InvokeListeners(close_event_listeners_, [](v8::Isolate* isolate,
                                               v8::Local<v8::Context> context,
                                               v8::Local<v8::Object> event_obj) {});
  }

  void OnError(int code, const std::string& message) override {
    InvokeListeners(error_event_listeners_, [code, message](v8::Isolate* isolate,
                                                            v8::Local<v8::Context> context,
                                                            v8::Local<v8::Object> event_obj) {
      event_obj->Set(context, v8String("code"), v8::Integer::New(isolate, code));
      event_obj->Set(context, v8String("message"), v8String(message));
    });
  }

  void OnMessage(void* data, std::size_t bytes) override {
    InvokeListeners(message_event_listeners_, [data, bytes](v8::Isolate* isolate,
                                                            v8::Local<v8::Context> context,
                                                            v8::Local<v8::Object> event_obj) {
      v8::Local<v8::ArrayBuffer> buffer = v8::ArrayBuffer::New(isolate, data, bytes);
      event_obj->Set(context, v8String("data"), buffer);
    });
  }

  // Adds the given |listener| as an event listener of the given |type|.
  void addEventListener(EventType type, v8::Local<v8::Function> listener) {
    std::vector<v8PersistentFunctionReference>* listeners = nullptr;

    switch (type) {
      case EventType::kClose:
        listeners = &close_event_listeners_;
        break;
      case EventType::kError:
        listeners = &error_event_listeners_;
        break;
      case EventType::kMessage:
        listeners = &message_event_listeners_;
        break;
    }

    CHECK(listeners);

    listeners->push_back(v8::Persistent<v8::Function>(v8::Isolate::GetCurrent(), listener));
  }

  // Removes the given |listener| as an event listener of the given |type|.
  void removeEventListener(EventType type, v8::Local<v8::Function> listener) {
    std::vector<v8PersistentFunctionReference>* listeners = nullptr;

    switch (type) {
      case EventType::kClose:
        listeners = &close_event_listeners_;
        break;
      case EventType::kError:
        listeners = &error_event_listeners_;
        break;
      case EventType::kMessage:
        listeners = &message_event_listeners_;
        break;
    }

    CHECK(listeners);

    for (auto iter = listeners->begin(); iter != listeners->end();) {
      const v8PersistentFunctionReference& ref = *iter;

      if (ref == listener)
        iter = listeners->erase(iter);
      else
        iter++;
    }
  }
  
 private:
  using v8PersistentFunctionReference = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;

  // Called when a Streamer instance has been garbage collected by the v8 engine.
  static void OnGarbageCollected(const v8::WeakCallbackInfo<SocketBindings>& data) {
    SocketBindings* instance = data.GetParameter();
    instance->object_.Reset();
    delete instance;
  }

  // Calls the |listeners|. The event object can be created by the |object_builder|, which will be
  // called under a valid handle and context scope.
  void InvokeListeners(std::vector<v8PersistentFunctionReference>& listeners,
                       std::function<void (v8::Isolate* isolate,
                                           v8::Local<v8::Context> context,
                                           v8::Local<v8::Object> event_obj)> object_builder) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();

    ScopedExceptionSource source("socket event");
    {
      std::shared_ptr<Runtime> runtime = Runtime::FromIsolate(isolate);
      if (!runtime)
        return;  // the runtime is being shut down

      v8::HandleScope handle_scope(runtime->isolate());

      v8::Local<v8::Context> context = runtime->context();
      v8::Context::Scope context_scope(context);

      v8::Local<v8::Object> event_obj = v8::Object::New(isolate);
      object_builder(isolate, context, event_obj);

      v8::Local<v8::Value> arguments[1];
      arguments[0] = event_obj;

      for (const v8PersistentFunctionReference& ref : listeners) {
        v8::Local<v8::Function> function = v8::Local<v8::Function>::New(isolate, ref);
        if (function.IsEmpty()) {
          LOG(WARNING) << "[v8] Unable to coerce the persistent funtion to a local for socket event";
          continue;
        }

        Call(isolate, function, arguments, 1u);
      }
    }
  }

  v8::Persistent<v8::Object> object_;
  std::unique_ptr<socket::Socket> socket_;

  std::unique_ptr<Promise> connection_promise_;

  // Map of event type to list of event listeners, stored as persistent references to v8 functions.
  std::vector<v8PersistentFunctionReference> close_event_listeners_;
  std::vector<v8PersistentFunctionReference> error_event_listeners_;
  std::vector<v8PersistentFunctionReference> message_event_listeners_;

  DISALLOW_COPY_AND_ASSIGN(SocketBindings);
};

// Returns the SocketBindings* instance from a JavaScript object, if any.
SocketBindings* GetBindingsFromObject(v8::Local<v8::Object> object) {
  if (object.IsEmpty() || object->InternalFieldCount() != 1) {
    ThrowException("Expected a Socket instance to be the |this| of the call.");
    return nullptr;
  }

  return static_cast<SocketBindings*>(object->GetAlignedPointerFromInternalField(0));
}

// Returns the socket::Socket object from a JavaScript object, if any.
socket::Socket* GetSocketFromObject(v8::Local<v8::Object> object) {
  SocketBindings* bindings = GetBindingsFromObject(object);
  if (bindings)
    return bindings->socket();

  return nullptr;
}

// -------------------------------------------------------------------------------------------------

bool ConvertStateToString(socket::State state, v8::Local<v8::String>* string) {
  switch (state) {
    case socket::State::kConnected:
      *string = v8String("connected");
      return true;

    case socket::State::kConnecting:
      *string = v8String("connecting");
      return true;

    case socket::State::kDisconnecting:
      *string = v8String("disconnecting");
      return true;

    case socket::State::kDisconnected:
      *string = v8String("disconnected");
      return true;
  }

  return false;
}

bool ConvertStringToEventType(v8::Local<v8::Value> value, SocketBindings::EventType* event_type) {
  if (!value->IsString())
    return false;

  std::string event_type_str = toString(value);
  std::transform(event_type_str.begin(), event_type_str.end(), event_type_str.begin(), ::tolower);

  if (event_type_str == "close") {
    *event_type = SocketBindings::EventType::kClose;
    return true;
  }

  if (event_type_str == "error") {
    *event_type = SocketBindings::EventType::kError;
    return true;
  }

  if (event_type_str == "message") {
    *event_type = SocketBindings::EventType::kMessage;
    return true;
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

// Socket.prototype.constructor(string protocol)
void SocketBindingsCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  if (!arguments.IsConstructCall()) {
    ThrowException("unable to construct Socket: must only be used as a constructor.");
    return;
  }

  if (arguments.Length() < 1) {
    ThrowException("unable to construct Socket: 1 argument required, but none provided.");
    return;
  }

  socket::SocketProtocol protocol;
  if (!socket::ParseSocketProtocol(arguments[0], &protocol)) {
    ThrowException("unable to construct Socket: invalid protocol given for argument 1");
    return;
  }

  std::unique_ptr<socket::BaseSocket> socket;
  switch (protocol) {
    case socket::SocketProtocol::kTCP:
      socket = std::make_unique<socket::TcpSocket>();
      break;
  }

  SocketBindings* instance = new SocketBindings(std::move(socket));
  instance->WeakBind(v8::Isolate::GetCurrent(), arguments.Holder());

  arguments.Holder()->SetAlignedPointerInInternalField(0, instance);
}

// Promise<boolean> Socket.prototype.open(SocketOpenOptions options)
void SocketOpenCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  socket::Socket* socket = GetSocketFromObject(arguments.Holder());
  if (!socket)
    return;

  if (socket->state() != socket::State::kDisconnected) {
    ThrowException("unable to call open(): the socket is already connected.");
    return;
  }

  if (arguments.Length() < 1) {
    ThrowException("unable to call open(): 1 arguments required, but none provided.");
    return;
  }

  socket::SocketOpenOptions options;
  if (!socket::ParseSocketOpenOptions(arguments[0], &options))
    return;

  std::shared_ptr<Promise> promise = std::make_shared<Promise>();
  v8::Local<v8::Promise> v8Promise = promise->GetPromise();

  socket->Open(std::move(options), std::move(promise));

  arguments.GetReturnValue().Set(v8Promise);
}

// Promise<boolean> Socket.prototype.write(ArrayBuffer data);
void SocketWriteCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  socket::Socket* socket = GetSocketFromObject(arguments.Holder());
  if (!socket)
    return;

  if (arguments.Length() < 1) {
    ThrowException("unable to call write(): 1 argument required, but none provided.");
    return;
  }

  v8::ArrayBuffer::Contents contents;
  if (arguments[0]->IsArrayBuffer()) {
    contents = v8::Local<v8::ArrayBuffer>::Cast(arguments[0])->GetContents();
  } else if (arguments[0]->IsTypedArray()) {
    contents = v8::Local<v8::TypedArray>::Cast(arguments[0])->Buffer()->GetContents();
  } else {
    ThrowException("unable to call write(): expected an ArrayBuffer or TypedArray for the first "
                   "argument.");
    return;
  }

  std::shared_ptr<Promise> promise = std::make_shared<Promise>();
  v8::Local<v8::Promise> v8Promise = promise->GetPromise();

  socket->Write((uint8_t*)contents.Data(), contents.ByteLength(), std::move(promise));

  arguments.GetReturnValue().Set(v8Promise);
}

// Promise<void> Socket.prototype.close()
void SocketCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  socket::Socket* socket = GetSocketFromObject(arguments.Holder());
  if (!socket)
    return;

  std::shared_ptr<Promise> promise = std::make_shared<Promise>();
  v8::Local<v8::Promise> v8Promise = promise->GetPromise();

  socket->Close(std::move(promise));

  arguments.GetReturnValue().Set(v8Promise);
}

// Socket.prototype.addEventListener(string event, function listener);
void SocketAddEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  SocketBindings* instance = GetBindingsFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 2) {
    ThrowException("unable to call addEventListener(): 2 arguments required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to call addEventListener(): expected a string for the first argument.");
    return;
  }

  if (!arguments[1]->IsFunction()) {
    ThrowException("unable to call addEventListener(): expected a function for the second "
                   "argument.");
    return;
  }

  v8::Local<v8::Function> listener = v8::Local<v8::Function>::Cast(arguments[1]);

  SocketBindings::EventType event_type;
  if (!ConvertStringToEventType(arguments[0], &event_type)) {
    ThrowException("unable to call addEventListener(): invalid event tpye given for argument 1");
    return;
  }

  instance->addEventListener(event_type, listener);
}

// Socket.prototype.removeEventListener(string event, function listener);
void SocketRemoveEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  SocketBindings* instance = GetBindingsFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 2) {
    ThrowException("unable to call removeEventListener(): 2 arguments required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsString()) {
    ThrowException("unable to call removeEventListener(): expected a string for the first "
                   "argument.");
    return;
  }

  if (!arguments[1]->IsFunction()) {
    ThrowException("unable to call removeEventListener(): expected a function for the second "
      "argument.");
    return;
  }

  v8::Local<v8::Function> listener = v8::Local<v8::Function>::Cast(arguments[1]);

  SocketBindings::EventType event_type;
  if (!ConvertStringToEventType(arguments[0], &event_type)) {
    ThrowException("unable to call removeEventListener(): invalid event tpye given for argument 1");
    return;
  }

  instance->removeEventListener(event_type, listener);
}

// Socket.prototype.state [getter]
void SocketStateGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  socket::Socket* socket = GetSocketFromObject(info.This());
  if (!socket)
    return;

  v8::Local<v8::String> state;
  if (ConvertStateToString(socket->state(), &state))
    info.GetReturnValue().Set(state);
  else
    info.GetReturnValue().SetNull();
}

}  // namespace

SocketModule::SocketModule() = default;

SocketModule::~SocketModule() = default;

void SocketModule::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template =
      v8::FunctionTemplate::New(isolate, SocketBindingsCallback);

  v8::Local<v8::ObjectTemplate> instance_template = function_template->InstanceTemplate();
  instance_template->SetInternalFieldCount(1 /** for the native instance **/);

  v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("open"), v8::FunctionTemplate::New(isolate, SocketOpenCallback));
  prototype_template->Set(v8String("write"), v8::FunctionTemplate::New(isolate, SocketWriteCallback));
  prototype_template->Set(v8String("close"), v8::FunctionTemplate::New(isolate, SocketCloseCallback));

  prototype_template->Set(v8String("addEventListener"), v8::FunctionTemplate::New(isolate, SocketAddEventListenerCallback));
  prototype_template->Set(v8String("removeEventListener"), v8::FunctionTemplate::New(isolate, SocketRemoveEventListenerCallback));

  prototype_template->SetAccessor(v8String("state"), SocketStateGetter);

  global->Set(v8String("Socket"), function_template);
}

}  // namespace bindings
