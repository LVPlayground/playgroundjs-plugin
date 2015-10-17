// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/callback_hook.h"

#include "base/logging.h"
#include "plugin/arguments.h"
#include "plugin/callback_parser.h"
#include "plugin/pawn_helpers.h"
#include "plugin/sdk/amx.h"
#include "plugin/sdk/plugincommon.h"
#include "third_party/subhook/subhook.h"

namespace plugin {

namespace {

// Type definition of the amx_Exec function that the CallbackHook will hook.
typedef int AMXAPI(*amx_Exec_t)(AMX* amx, cell* retval, int index);

// Global instance of the CallbackHook instance, should only be accessed by amx_exec_Hook().
CallbackHook* g_callback_hook = nullptr;

int amx_Exec_hook(AMX* amx, int* retval, int index) {
  return g_callback_hook->OnExecute(amx, retval, index);
}

}  // namespace

CallbackHook::CallbackHook(Delegate* delegate, const std::shared_ptr<CallbackParser>& callback_parser)
    : delegate_(delegate),
      callback_parser_(callback_parser),
      gamemode_(nullptr) {
  g_callback_hook = this;
}

CallbackHook::~CallbackHook() {
  g_callback_hook = nullptr;
}

bool CallbackHook::Install() {
  void* current_address = static_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec];
  if (current_address == nullptr) {
    LOG(ERROR) << "Invalid address found for the amx_Exec() function.";
    return false;
  }

  hook_.reset(new SubHook(current_address, (void*) amx_Exec_hook));
  if (!hook_->Install()) {
    LOG(ERROR) << "Unable to install a SubHook for the amx_Exec() function.";
    return false;
  }

  return true;
}

int CallbackHook::OnExecute(AMX* amx, int* retval, int index) {
  if (index == AMX_EXEC_MAIN) {
    OnGamemodeLoaded(amx);
  }  else if (gamemode_ == amx) {
    auto interceptor_iter = intercept_indices_.find(index);
    if (interceptor_iter != intercept_indices_.end()) {
      if (DoIntercept(amx, retval, *interceptor_iter->second))
        return AMX_ERR_NONE;
    }
  }

  // Trampoline back to the original amx_Exec function that we intercepted.
  return ((amx_Exec_t) hook_->GetTrampoline())(amx, retval, index);
}

bool CallbackHook::DoIntercept(AMX* amx, int* retval, const Callback& callback) {
  DCHECK(delegate_);

  static Arguments arguments;

  // Do a sanity check on the number of available arguments on the stack. We don't want to overrun
  // in stack space that's not reserved for invoking the actual callback.
  if (static_cast<size_t>(amx->paramcount) < callback.arguments.size()) {
    LOG(ERROR) << "Callback " << callback.name << " expected " << callback.arguments.size()
               << " arguments, got " << amx->paramcount << ".";
    return false;
  }

  size_t index = 0;

  arguments.clear();
  for (const auto& argument : callback.arguments) {
    switch (argument.second) {
    case ARGUMENT_TYPE_INT:
      arguments.AddInteger(argument.first, ReadIntFromStack(amx, index));
      break;
    case ARGUMENT_TYPE_FLOAT:
      arguments.AddFloat(argument.first, ReadFloatFromStack(amx, index));
      break;
    case ARGUMENT_TYPE_STRING:
      arguments.AddString(argument.first, ReadStringFromStack(amx, index));
      break;
    }

    ++index;
  }

  const bool result = delegate_->OnCallbackIntercepted(callback.name, arguments);
  if (result && retval)
    *retval = callback.return_value;

  if (callback.triggers_unload) {
    if (delegate_)
      delegate_->OnGamemodeChanged(nullptr);

    gamemode_ = nullptr;
  }

  return result;
}

typedef cell(AMX_NATIVE_CALL *AMX_NATIVE)(struct tagAMX *amx, cell *params);

void CallbackHook::OnGamemodeLoaded(AMX* amx) {
  int public_count = 0;
  if (amx_NumPublics(amx, &public_count) != AMX_ERR_NONE) {
    LOG(ERROR) << "Unable to read the number of public functions exported from the gamemode.";
    return;
  }

  char callback_name[sNAMEMAX + 1] = { 0 };
  for (int index = 0; index < public_count; ++index) {
    if (amx_GetPublic(amx, index, callback_name) != AMX_ERR_NONE) {
      LOG(ERROR) << "Unable to read the name of public function #" << index << " in the gamemode.";
      return;
    }

    const auto* callback = callback_parser_->Find(callback_name);
    if (callback != nullptr)
      intercept_indices_[index] = callback;
  }

  if (delegate_)
    delegate_->OnGamemodeChanged(amx);

  DCHECK(gamemode_ == nullptr);
  gamemode_ = amx;
}

}  // namespace plugin
