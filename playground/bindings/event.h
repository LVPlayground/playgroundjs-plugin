// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_EVENT_H_
#define PLAYGROUND_BINDINGS_EVENT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "plugin/callback.h"

namespace plugin {
class Arguments;
}

namespace v8 {
template <class T> class Local;
class Object;
class ObjectTemplate;
class Value;
}

namespace bindings {

// The Event class represents a dynamically created event based on SA-MP callbacks. The events
// will have named idiomatic to JavaScript and feature preventDefault functionality known from the
// DOM in Web development. Events can also conveniently be instantiated.
class Event {
 public:
  // Creates a new Event initialized from |callback|.
  static std::unique_ptr<Event> Create(const plugin::Callback& callback);

  // Returns whether the default of |value| has been prevented. Extensive checks will be done to
  // make sure that |value| actually is an Event instance.
  static bool DefaultPrevented(v8::Local<v8::Value> value);

  // Converts the SA-MP-esque |callback| name to an idiomatic JavaScript event type. Names will be
  // cached to prevent having to allocate memory for each lookup.
  static const std::string& ToEventType(const std::string& callback);

  ~Event();

  // Installs the prototype for this event on the |global| template.
  void InstallPrototype(v8::Local<v8::ObjectTemplate> global) const;

  // Creates a new instance of the event filled with the data from |arguments|. The returned object
  // may immediately be used to dispatch the event on the script's global scope.
  v8::Local<v8::Object> NewInstance(const plugin::Arguments& arguments) const;

 private:
  explicit Event(const plugin::Callback& callback);

  // Signature of the callback represented by this event.
  plugin::Callback callback_;

  // The event type associated with the callback.
  std::string event_type_;

  DISALLOW_COPY_AND_ASSIGN(Event);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_EVENT_H_