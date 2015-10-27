// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_MUTEX_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_MUTEX_H_

#if !defined(WIN32)
#include <pthread.h>
#endif

namespace mysql {

#if defined(WIN32)
typedef void* Mutex_t;
#else
typedef pthread_mutex_t Mutex_t;
#endif

class Mutex {
 public:
  Mutex();
  ~Mutex();

  void lock();
  void unlock();

 private:
  Mutex_t mutex_;
};

class ScopedMutex {
 public:
  explicit ScopedMutex(Mutex* mutex)
      : mutex_(mutex) {
    mutex_->lock();
  }

  ~ScopedMutex() {
    mutex_->unlock();
  }

 private:
  Mutex* mutex_;
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_MUTEX_H_
