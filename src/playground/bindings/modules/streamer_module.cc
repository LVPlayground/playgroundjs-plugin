// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer_module.h"

#include "base/logging.h"
#include "base/macros.h"
#include "bindings/modules/streamer/streamer.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

// Bindings class bridging between the JavaScript streamer object and the 
class StreamerBindings {
public:
  StreamerBindings(uint32_t max_visible, double stream_distance)
      : streamer_(max_visible, stream_distance) {}
  StreamerBindings();

  inline void Add(uint32_t id, double x, double y, double z) {
    streamer_.Add(id, x, y, z);
  }

  inline void Delete(uint32_t id) {
    streamer_.Delete(id);
  }

  inline void Clear() {
    streamer_.Clear();
  }

  void Stream(double x, double y, double z) {}

  size_t GetSize() const {
    return streamer_.size();
  }

  // Installs a weak reference to |object|, which is the JavaScript object that owns this instance.
  // A callback will be used to determine when it has been collected, so we can free up resources.
  void WeakBind(v8::Isolate* isolate, v8::Local<v8::Object> object) {
    object_.Reset(isolate, object);
    object_.SetWeak(this, OnGarbageCollected, v8::WeakCallbackType::kParameter);
  }

private:
  // Called when a Streamer instance has been garbage collected by the v8 engine.
  static void OnGarbageCollected(const v8::WeakCallbackInfo<StreamerBindings>& data) {
    StreamerBindings* instance = data.GetParameter();
    delete instance;
  }

  streamer::Streamer streamer_;

  v8::Persistent<v8::Object> object_;

  DISALLOW_COPY_AND_ASSIGN(StreamerBindings);
};

StreamerBindings* GetInstanceFromObject(v8::Local<v8::Object> object) {
  if (object.IsEmpty() || object->InternalFieldCount() != 1) {
    ThrowException("Expected a Streamer instance to be the |this| of the call.");
    return nullptr;
  }

  return static_cast<StreamerBindings*>(object->GetAlignedPointerFromInternalField(0));
}

// Streamer.prototype.constructor(number maxVisible, number streamDistance = 300)
void StreamerConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (!arguments.IsConstructCall()) {
    ThrowException("unable to construct Streamer: must only be used as a constructor.");
    return;
  }

  if (arguments.Length() < 1) {
    ThrowException("unable to construct Streamer: 1 argument required, but none provided.");
    return;
  }

  uint32_t max_visible = 0;
  double stream_distance = 300;

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to construct Streamer: expected a number for the first argument.");
    return;
  }

  max_visible = arguments[0]->Uint32Value();

  if (arguments.Length() >= 2) {
    if (!arguments[1]->IsNumber()) {
      ThrowException("unable to construct Streamer: expected a number for the second argument.");
      return;
    }

    stream_distance = arguments[1]->NumberValue();
  }

  StreamerBindings* instance = new StreamerBindings(max_visible, stream_distance);
  instance->WeakBind(v8::Isolate::GetCurrent(), arguments.Holder());

  arguments.Holder()->SetAlignedPointerInInternalField(0, instance);
}

// void Streamer.prototype.add(unsigned id, double x, double y, double z)
void StreamerAddCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 4) {
    ThrowException("unable to call add(): 4 argument required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the first argument.");
    return;
  }

  if (!arguments[1]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the second argument.");
    return;
  }

  if (!arguments[2]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the third argument.");
    return;
  }

  if (!arguments[3]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the fourth argument.");
    return;
  }

  instance->Add(arguments[0]->Uint32Value(),  // id
                arguments[1]->NumberValue(),  // x
                arguments[2]->NumberValue(),  // y
                arguments[3]->NumberValue()); // z
}

// void Streamer.prototype.delete(unsigned id)
void StreamerDeleteCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 1) {
    ThrowException("unable to call delete(): 1 argument required, but none provided.");
    return;
  }

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to call delete(): expected a number for the first argument.");
    return;
  }

  instance->Delete(arguments[0]->Uint32Value());
}

// void Streamer.prototype.clear()
void StreamerClearCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  instance->Clear();
}

// Promise<sequence<unsigned>> Streamer.prototype.stream(double x, double y, double z)
void StreamerStreamCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 3) {
    ThrowException("unable to call stream(): 3 argument required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the first argument.");
    return;
  }

  if (!arguments[1]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the second argument.");
    return;
  }

  if (!arguments[2]->IsNumber()) {
    ThrowException("unable to call add(): expected a number for the third argument.");
    return;
  }

  // TODO(Russell): Implement bindings for the |stream| method.
}

// Streamer.prototype.size
void StreamerSizeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  StreamerBindings* instance = GetInstanceFromObject(info.This());
  if (!instance)
    return;

  info.GetReturnValue().Set(instance->GetSize());
}

}  // namespace

StreamerModule::StreamerModule() {}

StreamerModule::~StreamerModule() {}

void StreamerModule::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, StreamerConstructorCallback);

  v8::Local<v8::ObjectTemplate> instance_template = function_template->InstanceTemplate();
  instance_template->SetInternalFieldCount(1 /** for the native instance **/);

  v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("add"), v8::FunctionTemplate::New(isolate, StreamerAddCallback));
  prototype_template->Set(v8String("delete"), v8::FunctionTemplate::New(isolate, StreamerDeleteCallback));
  prototype_template->Set(v8String("clear"), v8::FunctionTemplate::New(isolate, StreamerClearCallback));
  prototype_template->Set(v8String("stream"), v8::FunctionTemplate::New(isolate, StreamerStreamCallback));

  prototype_template->SetAccessor(v8String("ready"), StreamerSizeGetter);

  global->Set(v8String("Streamer"), function_template);
}

}  // namespace bindings
