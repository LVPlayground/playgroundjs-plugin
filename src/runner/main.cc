// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>

#include "pawn/amx.h"

#include "playground/base/logging.h"
#include "playground/plugin/sdk/amx.h"
#include "playground/plugin/sdk/plugincommon.h"
#include "playground/test_runner.h"
#include "playground/version.h"

// Exports used by the SA-MP plugin runtime, which PlaygroundJS uses to initialize itself.
PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData);
PLUGIN_EXPORT void PLUGIN_CALL Unload();
PLUGIN_EXPORT void PLUGIN_CALL ProcessTick();

namespace {

void* g_amx_exports[44] = { nullptr };

// Local implementation of the logprintf() function exported by the SA-MP server. Does not log the
// issued statements to a file (not even to server_log.txt).
void logprintf(char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  vprintf(format, arguments);
  va_end(arguments);

  putc('\n', stdout);
}

// The CallPublicFilterScript and CallPublicGameMode functions are not provided by this test
// runner. Log an error message to the console instead.
void UnimplementedCallPublicFunction(char* function_name) {
  LOG(ERROR) << "Unimplemented method. Cannot invoke " << function_name << ".";
}

// Creates the Pawn function exports based on the amx.c embedded in the test runner.
void* CreatePawnExports() {
  g_amx_exports[PLUGIN_AMX_EXPORT_Align16] = (void*)&amx_Align16;
  g_amx_exports[PLUGIN_AMX_EXPORT_Align32] = (void*)&amx_Align32;
  g_amx_exports[PLUGIN_AMX_EXPORT_Align64] = (void*)&amx_Align64;
  g_amx_exports[PLUGIN_AMX_EXPORT_Allot] = (void*)&amx_Allot;
  g_amx_exports[PLUGIN_AMX_EXPORT_Callback] = (void*)&amx_Callback;
  g_amx_exports[PLUGIN_AMX_EXPORT_Cleanup] = (void*)&amx_Cleanup;
  g_amx_exports[PLUGIN_AMX_EXPORT_Clone] = (void*)&amx_Clone;
  g_amx_exports[PLUGIN_AMX_EXPORT_Exec] = (void*)&amx_Exec;
  g_amx_exports[PLUGIN_AMX_EXPORT_FindNative] = (void*)&amx_FindNative;
  g_amx_exports[PLUGIN_AMX_EXPORT_FindPublic] = (void*)&amx_FindPublic;
  g_amx_exports[PLUGIN_AMX_EXPORT_FindPubVar] = (void*)&amx_FindPubVar;
  g_amx_exports[PLUGIN_AMX_EXPORT_FindTagId] = (void*)&amx_FindTagId;
  g_amx_exports[PLUGIN_AMX_EXPORT_Flags] = (void*)&amx_Flags;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetAddr] = (void*)&amx_GetAddr;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetNative] = (void*)&amx_GetNative;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetPublic] = (void*)&amx_GetPublic;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetPubVar] = (void*)&amx_GetPubVar;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetString] = (void*)&amx_GetString;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetTag] = (void*)&amx_GetTag;
  g_amx_exports[PLUGIN_AMX_EXPORT_GetUserData] = (void*)&amx_GetUserData;
  g_amx_exports[PLUGIN_AMX_EXPORT_Init] = (void*)&amx_Init;
  g_amx_exports[PLUGIN_AMX_EXPORT_InitJIT] = (void*)&amx_InitJIT;
  g_amx_exports[PLUGIN_AMX_EXPORT_MemInfo] = (void*)&amx_MemInfo;
  g_amx_exports[PLUGIN_AMX_EXPORT_NameLength] = (void*)&amx_NameLength;
  g_amx_exports[PLUGIN_AMX_EXPORT_NativeInfo] = (void*)&amx_NativeInfo;
  g_amx_exports[PLUGIN_AMX_EXPORT_NumNatives] = (void*)&amx_NumNatives;
  g_amx_exports[PLUGIN_AMX_EXPORT_NumPublics] = (void*)&amx_NumPublics;
  g_amx_exports[PLUGIN_AMX_EXPORT_NumPubVars] = (void*)&amx_NumPubVars;
  g_amx_exports[PLUGIN_AMX_EXPORT_NumTags] = (void*)&amx_NumTags;
  g_amx_exports[PLUGIN_AMX_EXPORT_Push] = (void*)&amx_Push;
  g_amx_exports[PLUGIN_AMX_EXPORT_PushArray] = (void*)&amx_PushArray;
  g_amx_exports[PLUGIN_AMX_EXPORT_PushString] = (void*)&amx_PushString;
  g_amx_exports[PLUGIN_AMX_EXPORT_RaiseError] = (void*)&amx_RaiseError;
  g_amx_exports[PLUGIN_AMX_EXPORT_Register] = (void*)&amx_Register;
  g_amx_exports[PLUGIN_AMX_EXPORT_Release] = (void*)&amx_Release;
  g_amx_exports[PLUGIN_AMX_EXPORT_SetCallback] = (void*)&amx_SetCallback;
  g_amx_exports[PLUGIN_AMX_EXPORT_SetDebugHook] = (void*)&amx_SetDebugHook;
  g_amx_exports[PLUGIN_AMX_EXPORT_SetString] = (void*)&amx_SetString;
  g_amx_exports[PLUGIN_AMX_EXPORT_SetUserData] = (void*)&amx_SetUserData;
  g_amx_exports[PLUGIN_AMX_EXPORT_StrLen] = (void*)&amx_StrLen;
  g_amx_exports[PLUGIN_AMX_EXPORT_UTF8Check] = (void*)&amx_UTF8Check;
  g_amx_exports[PLUGIN_AMX_EXPORT_UTF8Get] = (void*)&amx_UTF8Get;
  g_amx_exports[PLUGIN_AMX_EXPORT_UTF8Len] = (void*)&amx_UTF8Len;
  g_amx_exports[PLUGIN_AMX_EXPORT_UTF8Put] = (void*)&amx_UTF8Put;

  return g_amx_exports;
}

}  // namespace

int main(int argc, char** argv) {
  printf("=== Las Venturas Playground v%d.%d.%d (v8 %d.%d.%d) ==========================\n\n",
      playground::kPlaygroundVersionMajor, playground::kPlaygroundVersionMinor,
      playground::kPlaygroundVersionBuild, playground::kV8VersionMajor,
      playground::kV8VersionMinor, playground::kV8VersionBuild);

  if (!playground::RunPlaygroundTests(argc, argv))
    return 1;

  // Add a new line to separate the JavaScript tests from the native tests.
  putc('\n', stdout);

  {
    // TODO(Russell): Pass PLUGIN_DATA_AMX_EXPORTS in |plugin_data| with something sensible.
    void* plugin_data[] = {
      /* PLUGIN_DATA_LOGPRINTF */     logprintf,
      /* PLUGIN_DATA_AMX_EXPORTS */   CreatePawnExports(),
      /* PLUGIN_DATA_CALLPUBLIC_FS */ UnimplementedCallPublicFunction,
      /* PLUGIN_DATA_CALLPUBLIC_GM */ UnimplementedCallPublicFunction
    };

    Load(plugin_data);

    // TODO(Russell): Fire the OnGameModeInit event so that PlaygroundJS initializes itself.
    // TODO(Russell): Call ProcessTick() every few ms so that timers in PlaygroundJS work, as well
    //                as internal functionality that depends on it.

    Unload();
  }

  return 0;
}
