// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_PATH_MODULE_H_
#define PLAYGROUND_BINDINGS_MODULES_PATH_MODULE_H_

#include <include/v8.h>

#include "playground/bindings/runtime_modulator.h"

namespace bindings {

// The filesystem module implements a sub-set of the Node.js Path API:
// https://nodejs.org/api/path.html
//
// This functionality is not intended to be used by in-game features, but rather to be part of
// our support for the TypeScript compiler ahead of starting the gamemode.
class PathModule : public RuntimeModulator::SyntheticModule {
public:
  PathModule();
  ~PathModule() override;

  // RuntimeModulator::SyntheticModule implementation:
  std::vector<std::string> GetExportNames() const override;
  void RegisterExports(v8::Local<v8::Context> context,
    RuntimeModulator::SyntheticModuleRegistrar* registrar) const override;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_PATH_MODULE_H_
