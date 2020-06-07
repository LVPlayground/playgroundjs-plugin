// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer_module.h"

#include <boost/bind/bind.hpp>
#include <boost/lambda/bind.hpp>
#include <set>

#include "base/logging.h"
#include "base/macros.h"
#include "bindings/modules/streamer/streamer_host.h"
#include "bindings/promise.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

streamer::StreamerHost* GetHost() {
  return Runtime::FromIsolate(v8::Isolate::GetCurrent())->GetStreamerHost();
}

// Bindings class holding state for a Streamer instance.
class StreamerBindings {
 public:
   StreamerBindings(uint16_t max_visible, uint16_t max_distance)
       : streamer_id_(GetHost()->CreateStreamer(max_visible, max_distance)) {}

   ~StreamerBindings() = default;

   // Gets the unique streamer ID that's represented by this object.
   uint32_t streamer_id() const { return streamer_id_; }

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
    instance->object_.Reset();

    GetHost()->DeleteStreamer(instance->streamer_id());
    delete instance;
  }

  uint32_t streamer_id_;

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

// Streamer.setTrackedPlayers(Set playerIds)
void StreamerSetTrackedPlayersCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  if (arguments.Length() < 1) {
    ThrowException("unable to call setTrackedPlayers(): 1 argument required, but none provided.");
    return;
  }

  if (!arguments[0]->IsSet()) {
    ThrowException("unable to call setTrackedPlayers(): expected argument 1 to be a Set.");
    return;
  }

  auto context = arguments.GetIsolate()->GetCurrentContext();

  v8::Local<v8::Set> players_set = v8::Local<v8::Set>::Cast(arguments[0]);
  v8::Local<v8::Array> players_array = players_set->AsArray();
  
  std::set<uint16_t> players;
  if (!players_array.IsEmpty() && players_array->Length() > 0) {
    for (uint32_t index = 0; index < players_array->Length(); ++index) {
      v8::MaybeLocal<v8::Value> maybe_entry = players_array->Get(context, index);
      v8::Local<v8::Value> entry;

      if (!maybe_entry.ToLocal(&entry) || !entry->IsNumber())
        continue;

      v8::Local<v8::Number> entry_number = v8::Local<v8::Number>::Cast(entry);
      double entry_double = entry_number->Value();

      if (entry_double >= 0 && entry_double <= 1000)
        players.insert(static_cast<uint16_t>(entry_double));
    }
  }

  Runtime::FromIsolate(arguments.GetIsolate())->GetStreamerHost()->SetTrackedPlayers(std::move(players));
}

// Streamer.prototype.constructor(number maxVisible, number streamDistance = 300)
void StreamerConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  if (!arguments.IsConstructCall()) {
    ThrowException("unable to construct Streamer: must only be used as a constructor.");
    return;
  }

  if (arguments.Length() < 1) {
    ThrowException("unable to construct Streamer: 1 argument required, but none provided.");
    return;
  }

  uint16_t max_visible = 0;
  uint16_t max_distance = 300;

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to construct Streamer: expected a number for the first argument.");
    return;
  }

  max_visible = static_cast<uint16_t>(arguments[0]->Uint32Value(context).ToChecked());

  if (arguments.Length() >= 2) {
    if (!arguments[1]->IsNumber()) {
      ThrowException("unable to construct Streamer: expected a number for the second argument.");
      return;
    }

    max_distance = static_cast<uint16_t>(arguments[1]->Uint32Value(context).ToChecked());
  }

  StreamerBindings* instance = new StreamerBindings(max_visible, max_distance);
  instance->WeakBind(v8::Isolate::GetCurrent(), arguments.Holder());

  arguments.Holder()->SetAlignedPointerInInternalField(0, instance);
}

// number Streamer.prototype.add(number x, number y, number z)
void StreamerAddCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 3) {
    ThrowException("unable to call add(): 3 argument required, but only " +
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

  uint32_t entity_id = GetHost()->Add(
      instance->streamer_id(),
      static_cast<float>(arguments[0]->NumberValue(context).ToChecked()),
      static_cast<float>(arguments[1]->NumberValue(context).ToChecked()),
      static_cast<float>(arguments[2]->NumberValue(context).ToChecked()));

  arguments.GetReturnValue().Set(entity_id);
}

// void Streamer.prototype.delete(number entityId)
void StreamerDeleteCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

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

  GetHost()->Delete(instance->streamer_id(), arguments[0]->Uint32Value(context).ToChecked());
}

// Promise<sequence<unsigned>> Streamer.prototype.stream()
void StreamerStreamCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  std::shared_ptr<Promise> promise = std::make_shared<Promise>();

  bool result = GetHost()->Stream(
      instance->streamer_id(),
      boost::lambda::bind([](std::shared_ptr<Promise> promise, std::set<uint32_t> entities) {
        

      }, promise, boost::lambda::_1));
  
  if (!result)
    promise->Reject(v8::Exception::TypeError(v8String("The streamer has been deleted.")));

  arguments.GetReturnValue().Set(promise->GetPromise());
}

}  // namespace

StreamerModule::StreamerModule() = default;

StreamerModule::~StreamerModule() = default;

void StreamerModule::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, StreamerConstructorCallback);
  function_template->Set(v8String("setTrackedPlayers"), v8::FunctionTemplate::New(isolate, StreamerSetTrackedPlayersCallback));

  v8::Local<v8::ObjectTemplate> instance_template = function_template->InstanceTemplate();
  instance_template->SetInternalFieldCount(1 /** for the native instance **/);

  v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("add"), v8::FunctionTemplate::New(isolate, StreamerAddCallback));
  prototype_template->Set(v8String("delete"), v8::FunctionTemplate::New(isolate, StreamerDeleteCallback));
  prototype_template->Set(v8String("stream"), v8::FunctionTemplate::New(isolate, StreamerStreamCallback));

  global->Set(v8String("Streamer"), function_template);
}

}  // namespace bindings
