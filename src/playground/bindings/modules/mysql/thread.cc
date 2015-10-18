// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/mysql/thread.h"

#include <sys/timeb.h>

#if defined(WIN32)
#include <Windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

namespace mysql {

// -----------------------------------------------------------------------------
// Host-thread methods.

void Thread::StartThread() {
  if (is_running_)
    return;

  start(); // start the thread.
  is_running_ = true;
}

void Thread::start() {
#ifdef WIN32
  unsigned long thread_id;
  thread_ = CreateThread(NULL, 0, &Thread::internalRun, this, 0, &thread_id);
#else
  pthread_create(&thread_, 0, &Thread::internalRun, this);
#endif
}

void Thread::StopThread() {
  if (is_running_)
    stop(); // this blocks until the thread is gone.
}

void Thread::stop() {
  request_shutdown_ = true;
#ifdef WIN32
  WaitForSingleObject(thread_, 1000);
#else
  pthread_join(thread_, 0);
#endif
  is_running_ = false;
}

// -----------------------------------------------------------------------------
// Client-thread methods.

void Thread::thread_sleep(unsigned int milliseconds) {
#ifdef WIN32
  Sleep(milliseconds);
#else
  usleep(milliseconds * 1000);
#endif
}

thread_return_type Thread::internalRun(void* thread) {
  static_cast<Thread*>(thread)->run();
  return 0;
}

int Thread::time() {
  timeb time;
  ftime(&time);

  return time.millitm + (time.time & 0xFFFFF) * 1000;
}

int Thread::timeSpan(int start) {
  int duration = time() - start;
  if (duration < 0)
    duration += 0x100000 * 1000;

  return duration;
}

}  // namespace mysql
