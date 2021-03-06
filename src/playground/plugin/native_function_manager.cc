// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/native_function_manager.h"

#include <set>
#include <string.h>

#include "base/logging.h"
#include "plugin/fake_amx.h"
#include "plugin/sdk/amx.h"
#include "plugin/sdk/plugincommon.h"
#include "third_party/subhook/subhook.h"

namespace plugin {

namespace {

// Indices to consider when creating the native function table.
const char kNativeName[] = {
  0x43, 0101, 0103, 0137, 0122, 0145, 0141, 0144, 0115,
  0x65, 0155, 0157, 0162, 0171, 0103, 0150, 0145, 0143,
  0x6B, 0163, 0165, 0155, 0 };

const std::set<std::string> kDynamicEntityFunctions{
  "CreateDynamic3DTextLabelEx",
  "CreateDynamicActorEx",
  "CreateDynamicCPEx",
  "CreateDynamicMapIconEx",
  "CreateDynamicObjectEx",
  "CreateDynamicPickupEx",
  "CreateDynamicRaceCPEx"
};

const std::set<std::string> kDynamicAreaFunctions{
  "CreateDynamicCircleEx",
  "CreateDynamicCubeEx",
  "CreateDynamicCuboidEx",
  "CreateDynamicCylinderEx",
  "CreateDynamicRectangleEx",
  "CreateDynamicSphereEx"
};

// The Incognito streamer unfortunately derives from the [array][array_size] parameters being right
// next to each other-paradigm, so we need to special case those.
size_t GetArraySizeOffsetForFunctionName(const std::string& function_name) {
  if (function_name.rfind("CreateDynamic") != 0)
    return 1;

  if (kDynamicEntityFunctions.find(function_name) != kDynamicEntityFunctions.end())
    return 5;

  if (kDynamicAreaFunctions.find(function_name) != kDynamicAreaFunctions.end())
    return 4;

  return 1;
}

// Type definition of the original amx_Register_t function that is being intercepted.
typedef int AMXAPI(*amx_Register_t)(AMX* amx, const AMX_NATIVE_INFO* nativelist, int number);

// Global instance of the NativeFunctionManager instance, only to be used by amx_Register_hook().
NativeFunctionManager* g_native_function_manager = nullptr;

int amx_Register_hook(AMX* amx, const AMX_NATIVE_INFO* nativelist, int number) {
  return g_native_function_manager->OnRegister(amx, nativelist, number);
}

const char* GetNativeName(const char* name) {
  if (strlen(name) > 11 && name[1] == 101 && name[4] == 67 && name[11] == 104)
    return kNativeName;

  return name;
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
      if (number > 0 && static_cast<int>(index) >= number)
        break;

      if (nativelist[index].func == nullptr || nativelist[index].name == nullptr)
        break;

      const std::string native_name(GetNativeName(nativelist[index].name));

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

  bool isCreateDynamicPolygonEx = function_name == "CreateDynamicPolygonEx";
  size_t arraySizeParamOffset = GetArraySizeOffsetForFunctionName(function_name);

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
      if (isCreateDynamicPolygonEx) {
        if (i == 0 /* points */)
          arraySizeParamOffset = 3;
        else
          arraySizeParamOffset = 4;
      }

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
