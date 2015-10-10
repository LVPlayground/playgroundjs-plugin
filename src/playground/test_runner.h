// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_TEST_RUNNER_H_
#define PLAYGROUND_TEST_RUNNER_H_

#include "base/export.h"

namespace playground {

// Runs all unit tests included in the Playground library. Command line arguments may be passed
// to change how tests are run, or apply filters to only run a subset of tests.
PLAYGROUND_EXPORT bool RunPlaygroundTests(int argc, char** argv);

}  // namespace playground

#endif  // PLAYGROUND_TEST_RUNNER_H_
