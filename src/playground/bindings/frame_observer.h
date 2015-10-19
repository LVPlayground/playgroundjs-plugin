// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_FRAME_OBSERVER_H_
#define PLAYGROUND_BINDINGS_FRAME_OBSERVER_H_

#include "playground/bindings/runtime.h"

namespace bindings {

// Frame observers have the ability to observe frames on the server without needing complicated
// machinery to keep track of this themselves. Advised usage is through the ScopedFrameObserver.
class FrameObserver {
 public:
  // To be called for each frame on the server.
  virtual void OnFrame() = 0;

  virtual ~FrameObserver() {}
};

// Scoped frame observer that will automatically register itself when created, and unregister
// itself when the instance is out of scope again (e.g. when the class is destroyed).
class ScopedFrameObserver {
 public:
  explicit ScopedFrameObserver(FrameObserver* observer)
      : observer_(observer) {
    Runtime::FromIsolate(v8::Isolate::GetCurrent())->AddFrameObserver(observer);
  }

  ~ScopedFrameObserver() {
    Runtime::FromIsolate(v8::Isolate::GetCurrent())->RemoveFrameObserver(observer_);
  }

 private:
  FrameObserver* observer_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_FRAME_OBSERVER_H_
