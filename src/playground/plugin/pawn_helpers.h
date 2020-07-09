// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_PAWN_HELPERS_H_
#define PLAYGROUND_PLUGIN_PAWN_HELPERS_H_

#include <stdint.h>
#include <string>
#include <vector>

typedef struct tagAMX AMX;

namespace plugin {

// Reads an array from the Pawn stack at |array_index| of runtime |amx|, also accessing the Pawn
// heap as that's where arrays are stored.
const std::vector<uint32_t>& ReadArrayFromAmx(
    AMX* amx, int array_index, size_t array_size, std::vector<uint32_t>* buffer);

// Reads an integer from the Pawn stack at |index| of runtime |amx|.
int ReadIntFromStack(AMX* amx, int index);

// Reads a float from the Pawn stack at |index| of runtime |amx|.
float ReadFloatFromStack(AMX* amx, int index);

// Reads a string from the Pawn stack at |string_index| of runtime |amx|, also accessing the Pawn
// heap as that's where strings are stored.
const std::string& ReadStringFromAmx(AMX* amx, int string_index, std::string* buffer);

}  // namespace

#endif  // PLAYGROUND_PLUGIN_PAWN_HELPERS_H_
