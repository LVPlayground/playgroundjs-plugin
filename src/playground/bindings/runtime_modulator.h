// Copyright 2017 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_RUNTIME_MODULATOR_H_
#define PLAYGROUND_BINDINGS_RUNTIME_MODULATOR_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <include/v8.h>

#include "base/file_path.h"
#include "base/macros.h"

namespace bindings {

// Implements the module loading semantics that power usage of ES Modules.
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/import
class RuntimeModulator {
 public:
  // Interface that provides the ability to register methods on a synthetic module.
  class SyntheticModuleRegistrar {
   public:
    virtual ~SyntheticModuleRegistrar() = default;

    // Registers an export named |name| to be served by the given |value|.
    virtual void RegisterExport(const std::string& name, v8::Local<v8::Value> value) = 0;
  };

  // Interface that needs to be implemented by synthetic modules, whose functionality is
  // provided by and implemented in C++ rather than JavaScript.
  class SyntheticModule {
   public:
    virtual ~SyntheticModule() = default;

    // Returns a vector with the names this module exports.
    virtual std::vector<std::string> GetExportNames() const = 0;

    // Should register all the exports previously declared by GetExportedNames.
    virtual void RegisterExports(v8::Local<v8::Context> context,
                                 SyntheticModuleRegistrar* registrar) const = 0;
  };

  // Called by v8 when a module has to be loaded. The |referrer| contains the script
  // or module that initiated the load. The |specifier| is the name or path that is to
  // be loaded. Returns a Promise that will be resolved with the module namespace object.
  static v8::MaybeLocal<v8::Promise> ImportModuleDynamicallyCallback(
      v8::Local<v8::Context> context,
      v8::Local<v8::ScriptOrModule> referrer,
      v8::Local<v8::String> specifier);

  RuntimeModulator(v8::Isolate* isolate, const base::FilePath& root);
  ~RuntimeModulator();

  // Adds a new synthetic module with the given |name|.
  void AddSyntheticModule(const std::string& name, std::unique_ptr<SyntheticModule> module);

  // Loads the module identified by the |specifier| as the top-level module.
  v8::MaybeLocal<v8::Promise> LoadModule(v8::Local<v8::Context> context,
                                         const base::FilePath& referrer,
                                         const std::string& specifier);

  // Gets the module identified by the |specifier|, if any.
  v8::MaybeLocal<v8::Module> GetModule(v8::Local<v8::Context> context,
                                       v8::Local<v8::Module> referrer,
                                       const std::string& specifier);

  // Returns a pointer to the SyntheticModule instance for the given |specifier|, if any.
  SyntheticModule* GetSyntheticModulePtr(const std::string& specifier) const;

  // Clears the cached modules whose path matches |prefix|.
  void ClearCache(const std::string& prefix);

 private:
  // Aims to resolve the |resolver| with the module namespace object for a module
  // identified by |specifier|. If such a module is already loaded it will be
  // reused, otherwise it will be created.
  void ResolveOrCreateModule(v8::Local<v8::Context> context,
                             v8::Local<v8::Promise::Resolver> resolver,
                             const base::FilePath& referrer,
                             const std::string& specifier);

  // Gets the module that is identified by the given |path|.
  v8::MaybeLocal<v8::Module> GetModule(const base::FilePath& path);

  // Creates a new module that is identified by the given |path|.
  v8::MaybeLocal<v8::Module> CreateModule(v8::Local<v8::Context> context,
                                          const base::FilePath& path);

  // Resolves the path for the given |specifier| against the |referrer| script and the
  // |root_| that employs this modulator.
  bool ResolveModulePath(const base::FilePath& referrer,
                         const std::string& specifier,
                         base::FilePath* path) const;

  // Reads the file identified by |path| and writes the result to |contents|. Returns
  // whether the file could be read correctly. Failures must be handled.
  bool ReadFile(const base::FilePath& path,
                v8::Local<v8::String>* contents);

  // Returns the synthetic module identified by |specifier|.
  v8::MaybeLocal<v8::Module> GetSyntheticModule(const std::string& specifier);

  // Returns whether the given |specifier| is a synthetic module.
  bool IsSyntheticModule(const std::string& specifier) const;

  // Evaluates the given |module| on the given |context|.
  static v8::MaybeLocal<v8::Value> EvaluateSyntheticModule(v8::Local<v8::Context> context,
                                                           v8::Local<v8::Module> module);

  v8::Isolate* isolate_;
  base::FilePath root_;

  // Map of synthetic modules that could be loaded.
  std::map<std::string, std::unique_ptr<SyntheticModule>> synthetic_modules_;
  std::map<std::string, v8::Global<v8::Module>> loaded_synthetic_modules_;

  // Map of paths to loaded v8::Module instances.
  std::map<base::FilePath, v8::Global<v8::Module>> modules_;

  DISALLOW_COPY_AND_ASSIGN(RuntimeModulator);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_RUNTIME_MODULATOR_H_
