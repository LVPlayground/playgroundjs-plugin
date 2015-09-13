// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime_options.h"

#include "bindings/test_helpers.h"
#include "gtest/gtest.h"

namespace bindings {
namespace {

// Tests that each individual option maps to its designated v8 command line flag when enabled.
TEST(RuntimeOptionsTest, FlagMappings) {
  RuntimeOptions options;
  memset(&options, 0, sizeof(RuntimeOptions));

  struct {
    bool* option;
    const char* const flag;
  } mappings[] = {
    { &options.strict_mode, "--use_strict " }
  };
  
  for (size_t index = 0; index < sizeof(mappings) / sizeof(mappings[0]); ++index) {
    *mappings[index].option = true;
    EXPECT_EQ(mappings[index].flag, RuntimeOptionsToArgumentString(options));
    *mappings[index].option = false;
  }
}

// Tests that the strict mode flag actually forces strict mode in the executed JavaScript.
TEST(RuntimeOptionsTest, StrictModeFlag) {
  RuntimeOptions options;
  options.strict_mode = true;

  // |this| in a function called from the global context does not point to the global object when
  // strict mode has been enabled on the runtime.
  const char* code = "(function() { return !this; })()";

  EXPECT_TRUE(ScriptRunner::Execute(options, code));
}

}  // namespace
}  // namespace bindings
