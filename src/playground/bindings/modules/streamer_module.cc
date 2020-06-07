// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer_module.h"

#include <set>

#include "base/logging.h"
#include "base/macros.h"
#include "bindings/modules/streamer/streamer.h"
#include "bindings/modules/streamer/streamer_host.h"
#include "bindings/promise.h"
#include "bindings/utilities.h"

namespace bindings {

namespace {

// Bindings class bridging between the JavaScript streamer object and the 
class StreamerBindings : public ::streamer::Streamer {
 public:
   StreamerBindings(uint32_t max_visible, double stream_distance)
       : Streamer(max_visible, stream_distance) {}

   ~StreamerBindings() override {}

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
    delete instance;
  }

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

  uint32_t max_visible = 0;
  double stream_distance = 300;

  if (!arguments[0]->IsNumber()) {
    ThrowException("unable to construct Streamer: expected a number for the first argument.");
    return;
  }

  max_visible = arguments[0]->Uint32Value(context).ToChecked();

  if (arguments.Length() >= 2) {
    if (!arguments[1]->IsNumber()) {
      ThrowException("unable to construct Streamer: expected a number for the second argument.");
      return;
    }

    stream_distance = arguments[1]->NumberValue(context).ToChecked();
  }

  StreamerBindings* instance = new StreamerBindings(max_visible, stream_distance);
  instance->WeakBind(v8::Isolate::GetCurrent(), arguments.Holder());

  arguments.Holder()->SetAlignedPointerInInternalField(0, instance);
}

// void Streamer.prototype.add(unsigned id, double x, double y, double z)
void StreamerAddCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

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

  instance->Add(arguments[0]->Uint32Value(context).ToChecked(),  // id
                arguments[1]->NumberValue(context).ToChecked(),  // x
                arguments[2]->NumberValue(context).ToChecked(),  // y
                arguments[3]->NumberValue(context).ToChecked()); // z
}

// void Streamer.prototype.optimise()
void StreamerOptimiseCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  instance->Optimise();
}

// void Streamer.prototype.delete(unsigned id)
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

  instance->Delete(arguments[0]->Uint32Value(context).ToChecked());
}

// void Streamer.prototype.clear()
void StreamerClearCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  instance->Clear();
}

// Promise<sequence<unsigned>> Streamer.prototype.stream(number visible, double x, double y, double z)
void StreamerStreamCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  auto context = arguments.GetIsolate()->GetCurrentContext();

  StreamerBindings* instance = GetInstanceFromObject(arguments.Holder());
  if (!instance)
    return;

  if (arguments.Length() < 4) {
    ThrowException("unable to call stream(): 4 argument required, but only " +
                   std::to_string(arguments.Length()) + " provided.");
    return;
  }

  if (!arguments[0]->IsNumber() || !arguments[0]->IsUint32()) {
    ThrowException("unable to call add(): expected a positive integer for the first argument.");
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
    ThrowException("unable to call add(): expected a number for the first argument.");
    return;
  }

  Promise promise;
  
  const auto& results = instance->Stream(arguments[0]->Uint32Value(context).ToChecked(),  // visible
                                         arguments[1]->NumberValue(context).ToChecked(),  // x
                                         arguments[2]->NumberValue(context).ToChecked(),  // y
                                         arguments[3]->NumberValue(context).ToChecked()); // z

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Array> id_array = v8::Array::New(isolate, results.size());

  for (size_t i = 0; i < results.size(); ++i)
    id_array->Set(context, i, v8::Number::New(isolate, static_cast<double>(results[i])));

  promise.Resolve(id_array);

  arguments.GetReturnValue().Set(promise.GetPromise());
}

// Streamer.prototype.size
void StreamerSizeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  StreamerBindings* instance = GetInstanceFromObject(info.This());
  if (!instance)
    return;

  info.GetReturnValue().Set(instance->size());
}

}  // namespace

StreamerModule::StreamerModule() {}

StreamerModule::~StreamerModule() {}

void StreamerModule::InstallPrototypes(v8::Local<v8::ObjectTemplate> global) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, StreamerConstructorCallback);
  function_template->Set(v8String("setTrackedPlayers"), v8::FunctionTemplate::New(isolate, StreamerSetTrackedPlayersCallback));

  v8::Local<v8::ObjectTemplate> instance_template = function_template->InstanceTemplate();
  instance_template->SetInternalFieldCount(1 /** for the native instance **/);

  v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
  prototype_template->Set(v8String("add"), v8::FunctionTemplate::New(isolate, StreamerAddCallback));
  prototype_template->Set(v8String("optimise"), v8::FunctionTemplate::New(isolate, StreamerOptimiseCallback));
  prototype_template->Set(v8String("delete"), v8::FunctionTemplate::New(isolate, StreamerDeleteCallback));
  prototype_template->Set(v8String("clear"), v8::FunctionTemplate::New(isolate, StreamerClearCallback));
  prototype_template->Set(v8String("stream"), v8::FunctionTemplate::New(isolate, StreamerStreamCallback));

  prototype_template->SetAccessor(v8String("ready"), StreamerSizeGetter);

  global->Set(v8String("Streamer"), function_template);
}

}  // namespace bindings
