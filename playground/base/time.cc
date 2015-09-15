// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/time.h"

#if defined(WIN32)
#include <Windows.h>
#endif

namespace base {

#if defined(WIN32)

double monotonicallyIncreasingTime() {
  static ULONGLONG begin_time = 0ull;
  static bool begin_time_initialized = false;

  FILETIME tm;

  // Use precise (<1us) timing for Windows 8 and above, normal (~1ms) on other versions.
#if defined(NTDDI_WIN8) && NTDDI_VERSION >= NTDDI_WIN8
  GetSystemTimePreciseAsFileTime(&tm);
#else
  GetSystemTimeAsFileTime(&tm);
#endif

  if (!begin_time_initialized) {
    begin_time = tm.dwHighDateTime;
    begin_time_initialized = true;
  }

  ULONGLONG time = ((ULONGLONG) (begin_time - tm.dwHighDateTime) << 32) | (ULONGLONG)tm.dwLowDateTime;
  return static_cast<double>(time) / 10000.0;
}

#else

#error monotonicallyIncreasingTime() is not implemented for this platform.

#endif

}  // namespace base
