// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/script_loader.h"

#include <fstream>
#include <streambuf>

#include "base/file_path.h"
#include "base/logging.h"
#include "bindings/runtime.h"

namespace bindings {

ScriptLoader::ScriptLoader(Runtime* runtime, const base::FilePath& script_directory)
    : runtime_(runtime),
      script_directory_(script_directory) {}

ScriptLoader::~ScriptLoader() {}

bool ScriptLoader::Load(const base::FilePath& file) {
  base::FilePath script_path = script_directory_.Append(file);

  LOG(INFO) << "Loading " << file.value() << "...";

  std::ifstream handle(script_path.value().c_str());
  if (!handle.is_open() || handle.fail())
    return false;

  Runtime::ScriptSource script;
  script.filename = file.value();
  script.source.assign((std::istreambuf_iterator<char>(handle)),
                       std::istreambuf_iterator<char>());
  
  v8::HandleScope handle_scope(runtime_->isolate());
  if (!runtime_->Execute(script))
    LOG(ERROR) << "Unable to load JavaScript file: " << file.value();
  else
    LOG(INFO) << "Successfully loaded " << file.value() << "!";

  return true;
}

}  // namespace bindings
