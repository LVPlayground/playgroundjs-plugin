// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/native_parameters.h"

#include "base/logging.h"
#include "plugin/pawn_helpers.h"
#include "plugin/sdk/amx.h"

namespace plugin {

NativeParameters::NativeParameters(AMX* amx, int32_t* params)
    : amx_(amx),
      params_(params) {
  DCHECK(amx);
  DCHECK(params);
}

NativeParameters::~NativeParameters() {}

size_t NativeParameters::count() const {
  return params_[0] / 4;
}

int32_t NativeParameters::GetInteger(size_t index) const {
  CHECK(index < count());
  return params_[index + 1];
}

float NativeParameters::GetFloat(size_t index) const {
  CHECK(index < count());
  return *(float*)&params_[index + 1];
}

const std::string& NativeParameters::GetString(size_t index, std::string* buffer) const {
  CHECK(index < count());
  CHECK(buffer);

  return ReadStringFromAmx(amx_, params_[index + 1], buffer);
}

}  // namespace plugin
