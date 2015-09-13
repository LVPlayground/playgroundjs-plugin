// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_RUNTIME_OPTIONS_H_
#define PLAYGROUND_BINDINGS_RUNTIME_OPTIONS_H_

#include <string>

namespace bindings {

// Options to be passed to the v8 runtime. These affect the way how scripts will be loaded,
// parsed and run. Options may be added or be deprecated when new versions of v8 get included.
struct RuntimeOptions {
  RuntimeOptions() = default;

  // Whether the JavaScript code should be run in strict mode.
  // http://www.ecma-international.org/ecma-262/6.0/#sec-strict-mode-code
  bool strict_mode = true;
};

// Converts |options| to an argument string that can be used to start the V8 runtime.
std::string RuntimeOptionsToArgumentString(const RuntimeOptions& options);

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_RUNTIME_OPTIONS_H_
