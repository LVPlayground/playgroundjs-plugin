// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/native_function_manager.h"

#include <string.h>

#include "base/logging.h"
#include "plugin/fake_amx.h"
#include "plugin/sdk/amx.h"
#include "plugin/sdk/plugincommon.h"
#include "third_party/subhook/subhook.h"

namespace plugin {

namespace {

// Type definition of the original amx_Register_t function that is being intercepted.
typedef int AMXAPI(*amx_Register_t)(AMX* amx, const AMX_NATIVE_INFO* nativelist, int number);

// Global instance of the NativeFunctionManager instance, only to be used by amx_Register_hook().
NativeFunctionManager* g_native_function_manager = nullptr;

int amx_Register_hook(AMX* amx, const AMX_NATIVE_INFO* nativelist, int number) {
  return g_native_function_manager->OnRegister(amx, nativelist, number);
}

}  // namespace

NativeFunctionManager::NativeFunctionManager()
    : fake_amx_(new FakeAMX) {
  g_native_function_manager = this;
}

NativeFunctionManager::~NativeFunctionManager() {
  g_native_function_manager = nullptr;
}

bool NativeFunctionManager::Install() {
  if (!pAMXFunctions)
    return true;  // testing

  void* current_address = static_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_Register];
  if (current_address == nullptr) {
    LOG(ERROR) << "Invalid address found for the amx_Register() function.";
    return false;
  }

  hook_.reset(new SubHook(current_address, (void*) amx_Register_hook));
  if (!hook_->Install()) {
    LOG(ERROR) << "Unable to install a SubHook for the amx_Register() function.";
    return false;
  }

  return true;
}

int NativeFunctionManager::OnRegister(AMX* amx, const AMX_NATIVE_INFO* nativelist, int number) {
  if (nativelist != nullptr) {
    for (size_t index = 0; ; ++index) {
      if (nativelist[index].func == nullptr || nativelist[index].name == nullptr)
        break;

      const std::string native_name(nativelist[index].name);

      // The Pawn interpreter iterates over unresolved functions in the gamemode, and finds them in
      // the functions that are being registered in the amx_Register() call. This means that the first
      // function registered for a given name will be used, rather than having later registrations
      // override previous ones. Simply drop out if a double-registration is observed.
      if (!FunctionExists(native_name))
        native_functions_[native_name] = static_cast<NativeFn*>(nativelist[index].func);
    }
  }

  // Trampoline back to the original amx_Register function that we intercepted.
  return ((amx_Register_t) hook_->GetTrampoline())(amx, nativelist, number);
}

bool NativeFunctionManager::FunctionExists(const std::string& function_name) const {
  return native_functions_.find(function_name) != native_functions_.end();
}

int NativeFunctionManager::CallFunction(const std::string& function_name,
                                        const char* format, void** arguments) {
  auto function_iter = native_functions_.find(function_name);
  if (function_iter == native_functions_.end()) {
    LOG(WARNING) << "Attempting to invoke unknown Pawn native " << function_name << ". Ignoring.";
    return -1;
  }

  AMX* amx = fake_amx_->amx();

  size_t param_count = format ? strlen(format) : 0;

  params_.resize(param_count + 1);
  params_[0] = param_count * sizeof(cell);

  // Early-return if there are no arguments required for this native invication.
  if (!param_count)
    return function_iter->second(amx, params_.data());

  size_t arraySizeParamOffset = 1;

  // The CreateDynamicObjectEx method unfortunately follows a non-sensical parameter order, which
  // makes it different from every other method that accepts an array (or a string). Thank you.
  if (param_count == 18 && function_name == "CreateDynamicObjectEx")
    arraySizeParamOffset = 5;

  auto amx_stack = fake_amx_->GetScopedStackModifier();
  DCHECK(arguments);

  // Process the existing parameters, either store them in |params_| or push them on the stack.
  for (size_t i = 0; i < param_count; ++i) {
    switch (format[i]) {
    case 'i':
      params_[i + 1] = *reinterpret_cast<cell*>(arguments[i]);
      break;
    case 'f':
      params_[i + 1] = amx_ftoc(*reinterpret_cast<float*>(arguments[i]));
      break;
    case 'r':
      params_[i + 1] = amx_stack.PushCell(*reinterpret_cast<cell*>(arguments[i]));
      break;
    case 's':
      params_[i + 1] = amx_stack.PushString(reinterpret_cast<char*>(arguments[i]));
      break;
    case 'a':
      if (format[i + arraySizeParamOffset] != 'i') {
        LOG(WARNING) << "Cannot invoke " << function_name << ": 'a' parameter must be followed by a 'i'.";
        return -1;
      }

      {
        int32_t size = *reinterpret_cast<int32_t*>(arguments[i + arraySizeParamOffset]);

        params_[i + 1] = amx_stack.PushArray(reinterpret_cast<cell*>(arguments[i]), size);
        if (arraySizeParamOffset == 1)
          params_[i + 1 + arraySizeParamOffset] = size;
      }

      if (arraySizeParamOffset == 1)
        ++i;

      break;
    }
  }

  const int return_value = function_iter->second(amx, params_.data());

  // Read back the values which may have been modified by the SA-MP server.
  for (size_t i = 0; i < param_count; ++i) {
    switch (format[i]) {
    case 'r':
      amx_stack.ReadCell(params_[i + 1], reinterpret_cast<cell*>(arguments[i]));
      break;
    case 'a':
      {
        char* data = reinterpret_cast<char*>(arguments[i]);
        int32_t size = *reinterpret_cast<int32_t*>(arguments[i + arraySizeParamOffset]);

        amx_stack.ReadArray(params_[i + 1], data, size);
      }
      break;
    }
  }

  return return_value;
}

}  // namespace plugin
