// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>

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
    void* plugin_data[20] = { nullptr };

    plugin_data[PLUGIN_DATA_LOGPRINTF] = logprintf;
    plugin_data[PLUGIN_DATA_AMX_EXPORTS] = nullptr;
    plugin_data[PLUGIN_DATA_CALLPUBLIC_FS] = UnimplementedCallPublicFunction;
    plugin_data[PLUGIN_DATA_CALLPUBLIC_GM] = UnimplementedCallPublicFunction;

    Load(plugin_data);

    // TODO(Russell): Fire the OnGameModeInit event so that PlaygroundJS initializes itself.
    // TODO(Russell): Call ProcessTick() every few ms so that timers in PlaygroundJS work, as well
    //                as internal functionality that depends on it.

    Unload();
  }

  return 0;
}
