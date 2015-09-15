// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/test_helpers.h"

#include <include/v8.h>

#include "base/file_path.h"
#include "base/logging.h"
#include "bindings/runtime.h"
#include "bindings/runtime_options.h"

namespace bindings {

// Directory in which data files for JavaScript tests will reside.
const char kTestDataDirectory[] = "test_data";

// static
bool ScriptRunner::Execute(const RuntimeOptions& options, const std::string& source) {
  std::unique_ptr<Runtime> runtime =
      std::make_unique<Runtime>(options,
                                base::FilePath::CurrentDirectory().Append(kTestDataDirectory),
                                nullptr /** delegate **/);

  Runtime::ScriptSource script(source);

  v8::HandleScope handle_scope(runtime->isolate());
  v8::Local<v8::Value> result;

  if (!runtime->Execute(script, &result))
    return false;

  if (result->IsBoolean())
    return result.As<v8::Boolean>()->Value();

  LOG(WARNING) << "Unable to convert the return value to a boolean.";
  return false;
}

}  // namespace bindings
