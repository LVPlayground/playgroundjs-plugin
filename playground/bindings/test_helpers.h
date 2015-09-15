// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_TEST_HELPERS_H_
#define PLAYGROUND_BINDINGS_TEST_HELPERS_H_

#include <string>
#include <stdint.h>

namespace bindings {

struct RuntimeOptions;

// Helper class that allows tests to conveniently execute a script and retrieve the return
// value of said script in the desired format.
class ScriptRunner {
 public:
  static bool Execute(const RuntimeOptions& options, const std::string& script);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_TEST_HELPERS_H_
