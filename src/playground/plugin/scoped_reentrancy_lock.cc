// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/scoped_reentrancy_lock.h"

#include <stdint.h>

namespace plugin {
namespace {

uint32_t g_reentrancy_depth = 0;

}  // namespace

ScopedReentrancyLock::ScopedReentrancyLock() {
  ++g_reentrancy_depth;
}

ScopedReentrancyLock::~ScopedReentrancyLock() {
  --g_reentrancy_depth;
}

// static
bool ScopedReentrancyLock::IsReentrant() {
  return g_reentrancy_depth > 0;
}

}  // namespace plugin
