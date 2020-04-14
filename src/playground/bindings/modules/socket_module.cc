// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket_module.h"

#include <algorithm>

#include "bindings/modules/socket/socket.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

// Bindings class bridging lifetime between the JavaScript Socket object and the C++ one.
class SocketBindings {
 public:
  SocketBindings(socket::Protocol protocol)
      : socket_(std::make_unique<socket::Socket>(protocol)) {}

  ~SocketBindings() = default;

  // Returns a raw pointer to the Socket object owned by this binding.
  socket::Socket* socket() { return socket_.get(); }

  // Installs a weak reference to |object|, which is the JavaScript object that owns this instance.
  // A callback will be used to determine when it has been collected, so we can free up resources.
  void WeakBind(v8::Isolate* isolate, v8::Local<v8::Object> object) {
    object_.Reset(isolate, object);
    object_.SetWeak(this, OnGarbageCollected, v8::WeakCallbackType::kParameter);
  }

 private:
  // Called when a Streamer instance has been garbage collected by the v8 engine.
  static void OnGarbageCollected(const v8::WeakCallbackInfo<SocketBindings>& data) {
    SocketBindings* instance = data.GetParameter();
    instance->object_.Reset();
    delete instance;
  }

  v8::Persistent<v8::Object> object_;
  std::unique_ptr<socket::Socket> socket_;

  DISALLOW_COPY_AND_ASSIGN(SocketBindings);
};

// Returns the socket::Socket object from a JavaScript object, if any.
socket::Socket* GetSocketFromObject(v8::Local<v8::Object> object) {
  if (object.IsEmpty() || object->InternalFieldCount() != 1) {
    ThrowException("Expected a Socket instance to be the |this| of the call.");
    return nullptr;
  }

  SocketBindings* bindings =
      static_cast<SocketBindings*>(object->GetAlignedPointerFromInternalField(0));
  if (bindings)
    return bindings->socket();

  return nullptr;
}

// -------------------------------------------------------------------------------------------------

// Conversion: socket::Protocol <--> string
bool ConvertStringToProtocol(v8::Local<v8::Value> value, socket::Protocol* protocol) {
  if (!value->IsString())
    return false;

  std::string protocol_string = toString(value);
  std::transform(
      protocol_string.begin(), protocol_string.end(), protocol_string.begin(), ::tolower);

  if (protocol_string == "tcp") {
    *protocol = socket::Protocol::kTcp;
    return true;
  }

  if (protocol_string == "udp") {
    *protocol = socket::Protocol::kUdp;
    return true;
  }

  return false;
}

bool ConvertProtocolToString(socket::Protocol protocol, v8::Local<v8::String>* string) {
  switch (protocol) {
    case socket::Protocol::kTcp:
      *string = v8String("tcp");
      return true;

    case socket::Protocol::kUdp:
      *string = v8String("udp");
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

  socket::Protocol protocol;

  if (!ConvertStringToProtocol(arguments[0], &protocol)) {
    ThrowException("unable to construct Socket: invalid protocol given for argument 1");
    return;
  }

  SocketBindings* instance = new SocketBindings(protocol);
  instance->WeakBind(v8::Isolate::GetCurrent(), arguments.Holder());

  arguments.Holder()->SetAlignedPointerInInternalField(0, instance);
}

// Socket.prototype.protocol [getter]
void SocketProtocolGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  socket::Socket* instance = GetSocketFromObject(info.This());
  if (!instance)
    return;

  v8::Local<v8::String> protocol;
  if (ConvertProtocolToString(instance->protocol(), &protocol))
    info.GetReturnValue().Set(protocol);
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
  prototype_template->SetAccessor(v8String("protocol"), SocketProtocolGetter);

  global->Set(v8String("Socket"), function_template);
}

}  // namespace bindings
