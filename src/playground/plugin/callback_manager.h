// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_CALLBACK_MANAGER_H_
#define PLAYGROUND_PLUGIN_CALLBACK_MANAGER_H_

#include <string>
#include <unordered_map>

typedef struct tagAMX AMX;

namespace plugin {

class CallbackManager {
 public:
  CallbackManager();
  ~CallbackManager();

  // To be called when the gamemode changes to a new value.
  void OnGamemodeChanged(AMX* gamemode);

  // Calls |function_name| with |arguments| on the Pawn script that identified as the gamemode.
  int CallPublic(const std::string& function_name, const char* format, void** arguments);

 private:
  AMX* gamemode_;

  // Cache for maintaining a mapping between callback names and their indices in |gamemode_|.
  std::unordered_map<std::string, int> callback_index_cache_;
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_CALLBACK_MANAGER_H_
