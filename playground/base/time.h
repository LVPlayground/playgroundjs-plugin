// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_TIME_H_
#define PLAYGROUND_BASE_TIME_H_

#include <stdint.h>

namespace base {

// Returns a monotonically increasing timestamp in milliseconds, with sub-millisecond precision.
double monotonicallyIncreasingTime();

}  // namespace base

#endif  // PLAYGROUND_BASE_TIME_H_
