// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime_options.h"

#include <sstream>

namespace bindings {

std::string RuntimeOptionsToArgumentString(const RuntimeOptions& options) {
  std::stringstream stream;

  if (options.strict_mode)
    stream << "--use_strict ";

  return stream.str();
}

}  // namespace bindings
