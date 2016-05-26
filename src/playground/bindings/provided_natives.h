// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_PROVIDED_NATIVES_H_
#define PLAYGROUND_BINDINGS_PROVIDED_NATIVES_H_

#include <stdint.h>
#include <string>
#include <unordered_map>

#include <include/v8.h>

#include "base/macros.h"
#include "plugin/native_parameters.h"

namespace bindings {

// JavaScript can provide a number of native functions to Pawn, all of which will be routed through
// this class. It will maintain references to the JavaScript functions implementing the natives.
class ProvidedNatives {
public:
  // Enumeration containing all the natives that are exposed to Pawn from JavaScript.
  enum class Function {
    TestFunction
  };

  ProvidedNatives();
  ~ProvidedNatives();

  // Gets the current instance of the ProvidedNatives class.
  static ProvidedNatives* GetInstance();

  // Registers the |fn| as handling the native called |name|. Returns whether it could be registered
  // successfully- the Function value associated with |name| will be found automatically.
  bool Register(const std::string& name, const std::string& signature, v8::Local<v8::Function> fn);

  // Calls the |fn| in JavaScript given the |parameters|.
  int32_t Call(Function fn, plugin::NativeParameters& params);

private:
  using v8PersistentFunctionReference = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;

  // Buffer to be used for converting Pawn string to JavaScript strings.
  std::string text_buffer_;

  // Mapping of [name] to the [native ID] of the natives known to this class.
  std::unordered_map<std::string, Function> natives_;

  struct StoredNative {
    size_t param_count, retval_count;
    std::string name, signature;
    v8PersistentFunctionReference reference;
  };

  // Mapping of [native ID] to the JavaScript function handling the native.
  std::unordered_map<Function, StoredNative> native_handlers_;

  DISALLOW_COPY_AND_ASSIGN(ProvidedNatives);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_PROVIDED_NATIVES_H_
