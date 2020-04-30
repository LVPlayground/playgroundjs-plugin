// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/fs_module.h"

namespace bindings {

FsModule::FsModule() = default;

FsModule::~FsModule() = default;

std::vector<std::string> FsModule::GetExportNames() const {
  return { "default" };
}

void FsModule::RegisterExports(v8::Local<v8::Context> context,
                               RuntimeModulator::SyntheticModuleRegistrar* registrar) const {
  registrar->RegisterExport("default", v8::Number::New(context->GetIsolate(), 42));
}

}  // namespace bindings
