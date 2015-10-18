// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/mysql/mutex.h"

#if defined(WIN32)
#include <Windows.h>
#endif

namespace mysql {

#if defined(WIN32)

Mutex::Mutex() {
  mutex_ = CreateMutexA(NULL, FALSE, "lvpMySQL");
}

Mutex::~Mutex() {
  CloseHandle(mutex_);
}

void Mutex::lock() {
  WaitForSingleObject(mutex_, INFINITE);
}

void Mutex::unlock() {
  ReleaseMutex(mutex_);
}

#else

Mutex::Mutex() {
  pthread_mutex_init(&mutex_, NULL);
}

Mutex::~Mutex() {
  pthread_mutex_destroy(&mutex_);
}

void Mutex::lock() {
  pthread_mutex_lock(&mutex_);
}

void Mutex::unlock() {
  pthread_mutex_unlock(&mutex_);
}

#endif

}  // namespace mysql
