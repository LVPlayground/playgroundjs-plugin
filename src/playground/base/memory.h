// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_MEMORY_H_
#define PLAYGROUND_BASE_MEMORY_H_

#include "base/logging.h"

#if !defined(UNLIKELY)
#if defined(COMPILER_GCC) || defined(__clang__)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define UNLIKELY(x) (x)
#endif  // defined(COMPILER_GCC)
#endif  // !defined(UNLIKELY)

namespace base {
extern bool g_debugMemoryAllocations;
}  // namespace base

#define LOG_ALLOC LOG_STREAM(Info, UNLIKELY(base::g_debugMemoryAllocations))

#endif  // PLAYGROUND_BASE_MEMORY_H_
