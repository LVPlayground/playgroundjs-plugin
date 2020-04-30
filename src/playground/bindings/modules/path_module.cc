// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/path_module.h"

namespace bindings {

PathModule::PathModule() = default;

PathModule::~PathModule() = default;

std::vector<std::string> PathModule::GetExportNames() const {
  return { "default" };
}

void PathModule::RegisterExports(
    v8::Local<v8::Context> context, RuntimeModulator::SyntheticModuleRegistrar* registrar) const {
  registrar->RegisterExport("default", v8::Number::New(context->GetIsolate(), 42));
}

}  // namespace bindings
