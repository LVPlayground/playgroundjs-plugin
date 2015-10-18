// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_THREAD_SAFE_QUEUE_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_THREAD_SAFE_QUEUE_H_

#include "playground/bindings/modules/mysql/mutex.h"

#include <queue>

namespace mysql {

template <typename Type>
class ThreadSafeQueue {
 public:
	ThreadSafeQueue()	{}

	void push(const Type& data) {
		ScopedMutex mutex(&mutex_);
		queue_.push(data);
	}

	void pop(Type& data) {
		ScopedMutex mutex(&mutex_);
    data = queue_.front();
		queue_.pop();
	}

	unsigned int size() {
		return queue_.size();
	}

 private:
	std::queue<Type> queue_;
	Mutex mutex_;
};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_THREAD_SAFE_QUEUE_H_
