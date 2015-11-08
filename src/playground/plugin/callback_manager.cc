// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/plugin/callback_manager.h"

#include "base/logging.h"
#include "plugin/callback_hook.h"
#include "plugin/sdk/amx.h"

namespace plugin {

CallbackManager::CallbackManager() 
    : gamemode_(nullptr) {}

CallbackManager::~CallbackManager() {}

void CallbackManager::OnGamemodeChanged(AMX* gamemode) {
  gamemode_ = gamemode;
  callback_index_cache_.clear();
}

int CallbackManager::CallPublic(const std::string& function_name, const char* format, void** arguments) {
  if (!gamemode_)
    return -1;

  int callback_index = -1;
  
  // Read the callback's index from the cache (O(n)), or find it in the |gamemode_| which is
  // an O(log n) operation. This does not guarantee that the callback is valid.
  auto callback_index_iter = callback_index_cache_.find(function_name);
  if (callback_index_iter == callback_index_cache_.end()) {
    if (amx_FindPublic(gamemode_, function_name.c_str(), &callback_index) != AMX_ERR_NONE) {
      LOG(WARNING) << "Unable to determine the callback index of " << function_name;
      return -1;
    }

    callback_index_cache_[function_name] = callback_index;
  } else {
    callback_index = callback_index_iter->second;
  }

  // Bail out if the public function does not exist in the |gamemode_|.
  if (callback_index == -1)
    return -1;

  size_t param_count = format ? strlen(format) : 0;
  int return_value = -1;

  // Parameters of the reference type ('r') are prohibited, since they're uncommon for callbacks and
  // would require significant implementation complexity. Let's just use return values.
  for (size_t param = 0; param < param_count; ++param) {
    if (format[param] == 'r') {
      LOG(WARNING) << "Unable to invoke " << function_name << ": illegal reference parameter at index " << param;
      return return_value;
    }
  }

  std::vector<cell> cleanup_list;

  // Parameters will have to be pushed in reverse order.
  for (int param = param_count - 1; param >= 0; --param) {
    if (param > 0 && format[param] == 'i' && format[param - 1] == 'a')
      continue;  // ignore array size parameters.

    switch (format[param]) {
    case 'i':
      amx_Push(gamemode_, *reinterpret_cast<cell*>(arguments[param]));
      break;
    case 'f':
      amx_Push(gamemode_, amx_ftoc(*reinterpret_cast<float*>(arguments[param])));
      break;
    case 's':
      {
        cell amx_addr, *physical_cell = nullptr;
        amx_PushString(gamemode_, &amx_addr, &physical_cell, reinterpret_cast<char*>(arguments[param]), 0, 0);
        cleanup_list.push_back(amx_addr);
      }
      break;
    case 'a':
      {
        if (format[param + 1] != 'i') {
          LOG(WARNING) << "Array parameters must be followed by their size while invoking " << function_name;
          goto cleanup;
        }

        int array_size = *reinterpret_cast<int*>(arguments[param + 1]);

        cell amx_addr, *physical_cell = nullptr;
        amx_PushArray(gamemode_, &amx_addr, &physical_cell, reinterpret_cast<cell*>(arguments[param]), array_size);
        cleanup_list.push_back(amx_addr);
      }
      break;
    default:
      LOG(ERROR) << "Invalid parameter type '" << format[param] << "' seen while invoking " << function_name;
      goto cleanup;
    }
  }

  // All data exists on the stack. Now actually invoke the function on the script. A scoped ignore
  // callback object is created to prevent infinite loops.
  {
    CallbackHook::ScopedIgnore scoped_ignore;
    if (amx_Exec(gamemode_, &return_value, callback_index) != AMX_ERR_NONE)
      LOG(WARNING) << "Unable to invoke " << function_name << ": AMX error occurred.";
  }

cleanup:
  // Cleanup all allocations made on the Pawn stack for strings and arrays.
  for (cell amx_addr : cleanup_list)
    amx_Release(gamemode_, amx_addr);

  return return_value;
}

}  // namespace plugin
