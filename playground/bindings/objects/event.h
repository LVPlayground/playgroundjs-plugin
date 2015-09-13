// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_OBJECTS_EVENT_H_
#define PLAYGROUND_BINDINGS_OBJECTS_EVENT_H_

#include "bindings/global_object.h"
#include "plugin/callback.h"

namespace plugin {
class Arguments;
}

namespace bindings {

// The Event class encapsulates the information necessary to create a JavaScript interface for a
// Pawn callback, as well as the ability to create an instance when the callback is invoked.
class Event : public GlobalObject {
 public:
  Event(Runtime* runtime, const plugin::Callback& callback);
  ~Event();

  // Creates an instance of this event based on |arguments|.
  v8::Local<v8::Object> CreateInstance(const plugin::Arguments& arguments);

  // Dynamically generates a prototype based on |callback_|.
  void InstallPrototype(v8::Local<v8::ObjectTemplate> global) override;

  // Returns whether the default action has been prevented for the event instance passed in |value|.
  static bool HasDefaultBeenPrevented(v8::Local<v8::Value> value);

 private:
  // Static callback for the preventDefault() function available for cancelable events. The flag
  // will be stored as an internal field on the instance, to make memory management easier.
  static void PreventDefault(const v8::FunctionCallbackInfo<v8::Value>& arguments);

  // Static callback for the defaultPrevented getter. Will return the value of the internal flag
  // when it exists, or |false| (the default value) otherwise.
  static void DefaultPrevented(const v8::FunctionCallbackInfo<v8::Value>& arguments);

  plugin::Callback callback_;

  std::string interface_name_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_OBJECTS_EVENT_H_
