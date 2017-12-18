// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_VERSION_H_
#define PLAYGROUND_VERSION_H_

#include <include/v8-version.h>

namespace playground {

const int kPlaygroundVersionMajor = 2;
const int kPlaygroundVersionMinor = 0;
const int kPlaygroundVersionBuild = 0;

const int kV8VersionMajor = V8_MAJOR_VERSION;
const int kV8VersionMinor = V8_MINOR_VERSION;
const int kV8VersionBuild = V8_BUILD_NUMBER;

}  // namespace playground

#endif  // PLAYGROUND_VERSION_H_
