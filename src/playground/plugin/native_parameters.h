// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_NATIVE_PARAMETERS_H_
#define PLAYGROUND_PLUGIN_NATIVE_PARAMETERS_H_

#include <stdint.h>

#include "base/macros.h"

typedef struct tagAMX AMX;

namespace plugin {

// Encapsulates the arguments passed by and returned to Pawn through a native function call.
class NativeParameters {
 public:
  NativeParameters(AMX* amx, int32_t* params);
  ~NativeParameters();

  // Gets the number of arguments passed to the function call.
  size_t count() const;

  int32_t GetInteger(size_t index) const;
  float GetFloat(size_t index) const;

 private:
  AMX* amx_;
  int32_t* params_;

  DISALLOW_COPY_AND_ASSIGN(NativeParameters);
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_NATIVE_PARAMETERS_H_
