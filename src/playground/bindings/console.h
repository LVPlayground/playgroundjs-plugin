// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_CONSOLE_H_
#define PLAYGROUND_BINDINGS_CONSOLE_H_

#include "base/macros.h"

namespace v8 {
class Context;
template <class T> class Local;
class ObjectTemplate;
class Value;
}

namespace bindings {

class Runtime;

// The Console object (and "console" instance) enable the script to log output to the console, which
// will also be represented in the SA-MP server log. The syntax follows browser implementations, and
// provides the following methods:
//
//     console.log(arg1[, arg2[...]])
//
// Additional methods may be added later. The arguments to console.log() will be printed to the
// console in the highest detail possible.
class Console {
 public:
  Console() = default;

  // Installs the Console prototype to the |global| template.
  void InstallPrototype(v8::Local<v8::ObjectTemplate> global) const;

  // Instantiates the Console object to provide a default instance as the |console| variable. This
  // is analogous to the behavior of the console property in browsers.
  void InstallObjects(v8::Local<v8::Context> context) const;

  // Outputs |value| to the console. It will be recursively converted to textual representations.
  void OutputValue(v8::Local<v8::Value> value) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(Console);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_CONSOLE_H_
