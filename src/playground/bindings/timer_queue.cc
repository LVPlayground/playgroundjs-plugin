// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/timer_queue.h"

#include <vector>
#include <include/v8.h>

#include "base/logging.h"
#include "base/time.h"
#include "bindings/exception_handler.h"
#include "bindings/promise.h"
#include "bindings/runtime.h"
#include "performance/scoped_trace.h"

namespace bindings {

bool operator<(const TimerQueue::TimerStorage& lhs, const TimerQueue::TimerStorage& rhs) {
  return lhs.execution_time > rhs.execution_time;
}

TimerQueue::TimerQueue(Runtime* runtime)
    : runtime_(runtime) {}

TimerQueue::~TimerQueue() {}

void TimerQueue::Add(std::shared_ptr<Promise> promise, int64_t time) {
  TimerStorage storage;
  storage.promise = promise;
  storage.execution_time = base::monotonicallyIncreasingTime() + time;

  timers_.push(storage);
}

void TimerQueue::Run() {
  if (!timers_.size())
    return;

  std::vector<std::shared_ptr<Promise>> execution_list;
  double current_time = base::monotonicallyIncreasingTime();

  while (timers_.size()) {
    const TimerStorage& top = timers_.top();
    if (top.execution_time > current_time)
      break;

    execution_list.push_back(top.promise);
    timers_.pop();
  }

  if (!execution_list.size())
    return;

  performance::ScopedTrace trace(performance::TIMER_EXECUTION_TOTAL);

  ScopedExceptionSource source("server frame");
  {
    v8::HandleScope handle_scope(runtime_->isolate());
    v8::Context::Scope context_scope(runtime_->context());

    v8::Local<v8::Value> null = v8::Null(runtime_->isolate());
    for (const auto& promise : execution_list)
      promise->Resolve(null);
  }
}

}  // namespace bindings
