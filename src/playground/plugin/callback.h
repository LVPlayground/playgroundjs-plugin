// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_CALLBACK_H_
#define PLAYGROUND_PLUGIN_CALLBACK_H_

#include <string>
#include <vector>
#include <utility>

namespace plugin {

// Recognized argument types. Other types will be considered to be an integer (which is the
// case for, for example, Pawn enumerations).
enum CallbackArgumentType {
  ARGUMENT_TYPE_INT,
  ARGUMENT_TYPE_FLOAT,
  ARGUMENT_TYPE_STRING
};

// Structure representing a parsed Callback with all arguments and known annotations. Note
// that unknown annotations will be silently ignored.
struct Callback {
  Callback() = default;
  Callback(const Callback& other) {
    name = other.name;
    arguments = other.arguments;
    cancelable = other.cancelable;
    deferred = other.deferred;
    return_value = other.return_value;
  }

  std::string name;
  std::vector<std::pair<std::string, CallbackArgumentType>> arguments;

  bool cancelable = false;
  bool deferred = false;
  int return_value = 0;
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_CALLBACK_H_
