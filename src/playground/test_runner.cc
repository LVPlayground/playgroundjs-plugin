// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "test_runner.h"

#include "base/file_path.h"
#include "gtest/gtest.h"

namespace playground {

bool RunPlaygroundTests(int argc, char** argv) {
  base::FilePath::Initialize();

  testing::InitGoogleTest(&argc, argv);
  return !RUN_ALL_TESTS();
}

}  // namespace playground
