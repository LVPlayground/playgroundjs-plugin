// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/native_parameters.h"

#include "base/logging.h"

namespace plugin {

NativeParameters::NativeParameters(AMX* amx, int32_t* params)
    : amx_(amx),
      params_(params) {
  DCHECK(amx);
  DCHECK(params);
}

NativeParameters::~NativeParameters() {}

}  // namespace plugin
