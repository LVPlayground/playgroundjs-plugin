// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include <stdio.h>

#include "playground/test_runner.h"
#include "playground/version.h"

int main(int argc, char** argv) {
  printf("=== Las Venturas Playground v%d.%d.%d (v8 %d.%d.%d) ==========================\n\n",
      playground::kPlaygroundVersionMajor, playground::kPlaygroundVersionMinor,
      playground::kPlaygroundVersionBuild, playground::kV8VersionMajor,
      playground::kV8VersionMinor, playground::kV8VersionBuild);

  if (!playground::RunPlaygroundTests(argc, argv))
    return 1;

  // TODO: Do something useful here.
  printf("\n");

  return 0;
}
