// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_THREAD_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_THREAD_H_

#if !defined(WIN32)
#include <pthread.h>
#endif

namespace mysql {

#if defined(WIN32)
  typedef void* thread_t;
  typedef unsigned long thread_return_type;
  #define thread_call_type __stdcall
#else
  typedef pthread_t thread_t;
  typedef void* thread_return_type;
  #define thread_call_type
#endif

class Thread {
 public:
  Thread() {}

  // Only to be called on the host side.
  void StartThread();
  void StopThread();

  bool running() const { return is_running_; }

 protected:
  // Will only be called on the client side.
  virtual void run() = 0;

  void thread_sleep(unsigned int milliseconds);

  inline bool shutdown_requested() const { return request_shutdown_; }

  int time();
  int timeSpan(int start);

 private:
  // Only to be called on the host side.
  void start();
  void stop();

  // Only to be called on the client side.
  static thread_return_type thread_call_type internalRun(void* thread);

  thread_t thread_;
  unsigned int thread_id_ = 0;
  bool is_running_ = false;
  bool request_shutdown_ = false;
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_THREAD_H_
