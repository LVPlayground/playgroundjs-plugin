// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_TIMER_QUEUE_H_
#define PLAYGROUND_BINDINGS_TIMER_QUEUE_H_

#include <stdint.h>
#include <memory>
#include <queue>

#include "base/macros.h"

namespace bindings {

class Promise;
class Runtime;

// Implements a priority queue for Promise-based timers. It provides an API that allows adding new
// items to the queue, as well as running the queue as-is. No context is required for doing so.
class TimerQueue {
 public:
  explicit TimerQueue(Runtime* runtime);
  ~TimerQueue();

  // Adds |promise| to the queue of timers to execute, to be executed after |time| ms.
  void Add(std::shared_ptr<Promise> promise, int64_t time);

  // Runs the timer queue - determines if there's anything that should be executed this cycle.
  void Run(double current_time);

 private:
  Runtime* runtime_;

  // Storage for the pending timers. May be copied following vector semantics.
  struct TimerStorage {
    std::shared_ptr<Promise> promise;
    double execution_time;
  };

  // Enable the comparison function to access the TimerStorage structure.
  friend bool operator<(const TimerStorage& lhs, const TimerStorage& rhs);

  // Priority queue for the timers that are to be executed.
  std::priority_queue<TimerStorage> timers_;

  DISALLOW_COPY_AND_ASSIGN(TimerQueue);
};

};

#endif  // PLAYGROUND_BINDINGS_TIMER_QUEUE_H_
