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

    /** require(script) **/
    let _function = script => {
      if (!_script_cache.hasOwnProperty(script))
        _script_cache[script] = requireImpl(script);
      return _script_cache[script];
    };

    /** require.clear(prefix) **/
    _function.clear = prefix => {
      Object.keys(_script_cache).forEach(script => {
        if (script.startsWith(prefix))
          delete _script_cache[script];
      });
    };

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
