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
  return amx_ctof(params_[index + 1]);
}

const std::string& NativeParameters::GetString(size_t index, std::string* buffer) const {
  CHECK(index < count());
  CHECK(buffer);

  return ReadStringFromAmx(amx_, params_[index + 1], buffer);
}

void NativeParameters::SetInteger(size_t index, int32_t value) {
  CHECK(index < count());

  cell* address;
  if (amx_GetAddr(amx_, params_[index + 1], &address) == AMX_ERR_NONE)
    *address = value;
}

void NativeParameters::SetFloat(size_t index, float value) {
  CHECK(index < count());

  cell* address;
  if (amx_GetAddr(amx_, params_[index + 1], &address) == AMX_ERR_NONE)
    *address = amx_ftoc(value);
}

}  // namespace plugin
