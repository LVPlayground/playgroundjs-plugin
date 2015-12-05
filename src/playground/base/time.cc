// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/time.h"

#include <stdint.h>

#if defined(WIN32)
#include <Windows.h>
#elif defined(LINUX)
#include <sys/time.h>
#include <time.h>
#endif

namespace base {

#if defined(WIN32)

// Difference between January 1st, 1601 and January 1st, 1970 in nanoseconds, to convert from
// the Windows file time epoch to the UNIX timestamp epoch.
const uint64_t kEpochDelta = 116444736000000000ull;

double monotonicallyIncreasingTime() {
  static uint64_t begin_time = 0ull;
  static bool set_begin_time = false;

  FILETIME tm;

  // Use precise (<1us) timing for Windows 8 and above, normal (~1ms) on other versions.
#if defined(NTDDI_WIN8) && NTDDI_VERSION >= NTDDI_WIN8
  GetSystemTimePreciseAsFileTime(&tm);
#else
  GetSystemTimeAsFileTime(&tm);
#endif

  uint64_t time = 0;
  time  |= tm.dwHighDateTime;
  time <<= 32;
  time  |= tm.dwLowDateTime;
  time  -= kEpochDelta;
  
  if (!set_begin_time) {
    set_begin_time = true;
    begin_time = time;
  }

  return static_cast<double>(time - begin_time) / 10000.0;
}

#elif defined(LINUX)

double monotonicallyIncreasingTime() {
  static uint64_t begin_time = 0ull;
  static bool set_begin_time = false;

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t time = static_cast<uint64_t>(ts.tv_sec) * 1000000000u + static_cast<uint64_t>(ts.tv_nsec);

  if (!set_begin_time) {
    set_begin_time = true;
    begin_time = time;
  }

  return static_cast<double>(time - begin_time) / 1000000.0;
}

#else

#error monotonicallyIncreasingTime() is not implemented for this platform.

#endif

}  // namespace base
