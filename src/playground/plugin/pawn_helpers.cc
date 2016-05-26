// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/pawn_helpers.h"

#include "base/logging.h"
#include "plugin/sdk/amx.h"

namespace plugin {

namespace {

template<typename T>
T ReadValueFromStack(AMX* amx, int offset) {
  static_assert(sizeof(T) == sizeof(cell), "sizeof(T) must be equal to typeof(cell).");

  AMX_HEADER* header = reinterpret_cast<AMX_HEADER*>(amx->base);
  unsigned char* data =
      amx->data == nullptr ? amx->base + header->dat
                           : amx->data;

  return *(T *)(data + amx->stk + offset * sizeof(cell));
}

}  // namespace

int ReadIntFromStack(AMX* amx, int index) {
  return ReadValueFromStack<int>(amx, index);
}

float ReadFloatFromStack(AMX* amx, int index) {
  return ReadValueFromStack<float>(amx, index);
}

const std::string& ReadStringFromAmx(AMX* amx, int string_index, std::string* buffer) {
  buffer->resize(0);

  cell* string_address = nullptr;

  if (amx_GetAddr(amx, string_index, &string_address) != AMX_ERR_NONE) {
    LOG(ERROR) << "Unable to read the address of a string argument.";
    return *buffer;
  }

  int string_length = 0;
  if (amx_StrLen(string_address, &string_length) != AMX_ERR_NONE) {
    LOG(ERROR) << "Unable to read the length of a string argument.";
    return *buffer;
  }

  if (!string_length)
    return *buffer;

  buffer->resize(string_length + 1, '\0');

  if (amx_GetString(&*buffer->begin(), string_address, 0, string_length + 1) != AMX_ERR_NONE) {
    LOG(ERROR) << "Unable to copy the string from the Pawn runtime.";
    return *buffer;
  }

  buffer->resize(string_length);
  return *buffer;
}

}  // namespace
