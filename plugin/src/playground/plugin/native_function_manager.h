// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_NATIVE_FUNCTION_MANAGER_H_
#define PLAYGROUND_PLUGIN_NATIVE_FUNCTION_MANAGER_H_

#include <stdarg.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

typedef struct tagAMX AMX;
typedef struct tagAMX_NATIVE_INFO AMX_NATIVE_INFO;

class SubHook;

namespace plugin {

class FakeAMX;

// The native function manager is responsible for maintaining a list of available native functions
// registered in the SA-MP server, as well as providing the ability to actually invoke any of these
// functions without needing an actual gamemode.
class NativeFunctionManager {
 public:
  NativeFunctionManager();
  ~NativeFunctionManager();

  // Installs the native function hook in the SA-MP server. Returns whether this was successful.
  bool Install();

  // Called when the amx_Register() function gets called either by another plugin, or by the SA-MP
  // server directly. Relies on the fact that plugins get initialized before the gamemodes.
  int OnRegister(AMX* amx, const AMX_NATIVE_INFO* nativelist, int number);

  // Returns whether a native named |function_name| exists in the Pawn runtime.
  bool FunctionExists(const std::string& function_name) const;

  // Calls |function_name| according to |format|, using |arguments| to fill in the |format|. Any
  // number of arguments are supported, and reference types will be stored back in the pointer.
  //
  // The following parameter formats for |format| are supported, with the accompanying type that
  // is expected to be available in |arguments|:
  //
  //   i (int*)      - 32-bit signed integer
  //   f (float*)    - 32-bit floating point
  //   r (void*)     - reference variable (will be read back)
  //   s (char*)     - zero-terminated string
  //   a (char*)     - array (must be followed by a '+' argument w/ the int32 size)
  //
  // Parameters of other types will result in a warning being thrown, and '-1' being returned.
  int CallFunction(const std::string& function_name, const char* format, void** arguments);

 private:
  using NativeFn = int32_t(AMX* amx, int32_t* params);

  // Map from the name of a native function to the pointer that represents said function. This map
  // will be complete before the first gamemode loads.
  std::unordered_map<std::string, NativeFn*> native_functions_;

  // Reusable parameter vector to prevent heap allocations for each function invocation.
  std::vector<int32_t> params_;

  // Rather than using a live mode to invoke methods on the SA-MP server and plugins, we fake an
  // AMX environment to minimize chances of disruption.
  std::unique_ptr<FakeAMX> fake_amx_;

  // The native function hook with which the amx_Register function will be intercepted.
  std::unique_ptr<SubHook> hook_;
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_NATIVE_FUNCTION_MANAGER_H_
