// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_SCRIPT_LOADER_H_
#define PLAYGROUND_BINDINGS_SCRIPT_LOADER_H_

#include <string>

#include "base/file_path.h"

namespace bindings {

class Runtime;

// The script loader is responsible for loading JavaScript files in the v8 runtime.
class ScriptLoader {
 public:
  explicit ScriptLoader(Runtime* runtime, const base::FilePath& script_directory);
  ~ScriptLoader();

  // Loads |filename| in the JavaScript runtime. Returns whether the file could be loaded
  // successfully. Errors will be outputted otherwise.
  bool Load(const base::FilePath& file);

 private:
  // Weak, owned by our owner. Instance of the runtime to operate on.
  Runtime* runtime_;

  // Base directory in which the script files will reside.
  base::FilePath script_directory_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_SCRIPT_LOADER_H_
