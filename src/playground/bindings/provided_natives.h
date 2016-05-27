// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_PROVIDED_NATIVES_H_
#define PLAYGROUND_BINDINGS_PROVIDED_NATIVES_H_

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <include/v8.h>

#include "base/macros.h"
#include "plugin/native_parameters.h"

namespace bindings {

// JavaScript can provide a number of native functions to Pawn, all of which will be routed through
// this class. It will maintain references to the JavaScript functions implementing the natives.
class ProvidedNatives {
public:
  ProvidedNatives();
  ~ProvidedNatives();

  // Gets the current instance of the ProvidedNatives class.
  static ProvidedNatives* GetInstance();

  // Sets the natives identified by the `natives.txt` file that may be handled by this class.
  void SetNatives(const std::vector<std::string>& natives);

  // Returns whether the native |name| has been provided by the JavaScript code.
  bool IsProvided(const std::string& name) const {
    return !!known_natives_.count(name);
  }

  // Registers the |fn| as handling the native called |name|. Returns whether it could be registered
  // successfully- the Function value associated with |name| will be found automatically.
  bool Register(const std::string& name, const std::string& signature, v8::Local<v8::Function> fn);

  // Calls the |fn| in JavaScript given the |parameters|.
  int32_t Call(const std::string& name, plugin::NativeParameters& params);

private:
  using v8PersistentFunctionReference = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;

  // Buffer to be used for converting Pawn string to JavaScript strings.
  std::string text_buffer_;

  // Set of the natives which are known to this class.
  std::unordered_set<std::string> known_natives_;
  
  struct StoredNative {
    size_t param_count, retval_count;
    std::string name, signature;
    v8PersistentFunctionReference reference;
  };

  // Mapping of function name to the JavaScript function handling the native.
  std::unordered_map<std::string, StoredNative> native_handlers_;

  DISALLOW_COPY_AND_ASSIGN(ProvidedNatives);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_PROVIDED_NATIVES_H_
