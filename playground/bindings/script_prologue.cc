// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/script_prologue.h"

namespace bindings {

// NOTE: Line breaks will be removed from these scripts before their execution in the
//       JavaScript virtual machine. Therefore you must NOT use line-level comments in this code,
//       as that will cause the first line of the user script to be ignored.

// Define the require() function on the global object, available to all.
const char kScriptPrologue[] = R"(
Object.defineProperty(self, 'require', {
  enumerable: true,
  configurable: false,
  writable: false,
  value: (() => {
    let _script_cache = {};
    let _script_mappings = {};

    /** require(script) **/
    let _function = (script) => {
      if (_script_mappings.hasOwnProperty(script))
        script = _script_mappings[script];
      if (!_script_cache.hasOwnProperty(script))
        _script_cache[script] = requireImpl(script);
      return _script_cache[script];
    };

    /** require.map(source, destination) **/
    _function.map = (script, destination) =>
      _script_mappings[script] = destination;
    return _function;
  })()
});
)";

const char kModulePrologue[] = R"(
(function() {
  let exports = {};
  let module = {};

  return (function(require, exports, module, global) {
)";

const char kModuleEpilogue[] = R"(
    ; return exports;
  })(require, exports, module, self);
})();
)";

}  // namespace bindings
