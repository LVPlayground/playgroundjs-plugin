// Copyright 2017 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_RUNTIME_MODULATOR_H_
#define PLAYGROUND_BINDINGS_RUNTIME_MODULATOR_H_

#include <map>
#include <string>

#include <include/v8.h>

#include "base/file_path.h"
#include "base/macros.h"

namespace bindings {

// Implements the module loading semantics that power usage of ES Modules.
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/import
class RuntimeModulator {
 public:
  // Returns whether the Modulator is enabled.
  static constexpr bool IsEnabled() { return true; }

  // Called by v8 when a module has to be loaded. The |referrer| contains the script
  // or module that initiated the load. The |specifier| is the name or path that is to
  // be loaded. Returns a Promise that will be resolved with the module namespace object.
  static v8::MaybeLocal<v8::Promise> ImportModuleDynamicallyCallback(
      v8::Local<v8::Context> context,
      v8::Local<v8::ScriptOrModule> referrer,
      v8::Local<v8::String> specifier);

  RuntimeModulator(v8::Isolate* isolate, const base::FilePath& root);
  ~RuntimeModulator();

  // Loads the module identified by the |specifier| as the top-level module.
  v8::MaybeLocal<v8::Promise> LoadModule(v8::Local<v8::Context> context,
                                         const std::string& referrer,
                                         const std::string& specifier);

  // Gets the module identified by the |specifier|, if any.
  v8::MaybeLocal<v8::Module> GetModule(v8::Local<v8::Context> context,
                                       v8::Local<v8::Module> referrer,
                                       const std::string& specifier);

 private:
  // Aims to resolve the |resolver| with the module namespace object for a module
  // identified by |specifier|. If such a module is already loaded it will be
  // reused, otherwise it will be created.
  void ResolveOrCreateModule(v8::Local<v8::Context> context,
                             v8::Local<v8::Promise::Resolver> resolver,
                             const std::string& referrer,
                             const std::string& specifier);

  // Gets the module that is identified by the given |path|.
  v8::MaybeLocal<v8::Module> GetModule(const base::FilePath& path);

  // Creates a new module that is identified by the given |path|.
  v8::MaybeLocal<v8::Module> CreateModule(v8::Local<v8::Context> context,
                                          const base::FilePath& path);

  // Resolves the path for the given |specifier| against the |referrer| script and the
  // |root_| that employs this modulator.
  bool ResolveModulePath(const std::string& referrer,
                         const std::string& specifier,
                         base::FilePath* path) const;

  // Reads the file identified by |path| and writes the result to |contents|. Returns
  // whether the file could be read correctly. Failures must be handled.
  bool ReadFile(const base::FilePath& path,
                v8::Local<v8::String>* contents);

  v8::Isolate* isolate_;
  base::FilePath root_;

  // TODO(Russell): This should be keyed on base::FilePath.
  std::map<std::string, v8::Global<v8::Module>> modules_;

  DISALLOW_COPY_AND_ASSIGN(RuntimeModulator);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_RUNTIME_MODULATOR_H_
