// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_ALLOCATOR_H_
#define PLAYGROUND_BINDINGS_ALLOCATOR_H_

#include <string>
#include <malloc.h>

#include <include/v8.h>

namespace bindings {

// Taken from src/samples/hello-world.cc from the v8 source.
class SimpleArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
  void* Allocate(size_t length) override {
    void* data = AllocateUninitialized(length);
    return data == NULL ? data : memset(data, 0, length);
  }

  void* AllocateUninitialized(size_t length) override {
    return malloc(length);
  }

  void Free(void* data, size_t) override {
    free(data);
  }
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_ALLOCATOR_H_
