// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_SCOPED_REENTRANCY_LOCK_H_
#define PLAYGROUND_PLUGIN_SCOPED_REENTRANCY_LOCK_H_

namespace plugin {

// Stack-scoped object that will guard against re-entrancy issues on the same Pawn runtime
// instance. For example, this could occur when a callback is being executed by JavaScript which
// then calls another event on the Pawn runtime, after which the original event will still be
// delivered. At that time, the stack will have been amended resulting in undefined behaviour.
class ScopedReentrancyLock {
public:
  ScopedReentrancyLock();
  ~ScopedReentrancyLock();

  // Returns whether calls into Pawn would cause reentrant behaviour.
  static bool IsReentrant();
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_SCOPED_REENTRANCY_LOCK_H_
