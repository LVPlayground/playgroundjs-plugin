// Copyright 2017 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime_modulator.h"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>

#include "base/logging.h"
#include "bindings/exception_handler.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

namespace fs = boost::filesystem;

namespace bindings {
namespace {

// Returns whether the |specifier| refers to a file on the HTTP(s) protocol.
bool IsHTTP(const std::string& specifier) {
  std::string lowercase_specifier(specifier.size(), ' ');
  std::transform(
      specifier.begin(), specifier.end(), lowercase_specifier.begin(), ::tolower);

  return !lowercase_specifier.find("http:") ||
         !lowercase_specifier.find("https:");
}

// Returns whether the file represented by |path| exists.
bool FileExists(const fs::path& path) {
  return fs::exists(path);
}

// Resolves the given |specifier| to a v8::Module instance. This method is not
// expected to fail, as all dependent modules should've been loaded during the
// LoadModule() phase. (Which recursively loads child modules.)
v8::MaybeLocal<v8::Module> ResolveModuleCallback(
    v8::Local<v8::Context> context,
    v8::Local<v8::String> specifier,
    v8::Local<v8::Module> referrer) {
  return Runtime::FromIsolate(context->GetIsolate())->GetModulator()->GetModule(
    context, referrer, toString(specifier));
}

}  // namespace

// static
// Powers dynamic import() calls rather than static, compile-time ones.
v8::MaybeLocal<v8::Promise> RuntimeModulator::ImportModuleDynamicallyCallback(
  v8::Local<v8::Context> context,
  v8::Local<v8::ScriptOrModule> referrer,
  v8::Local<v8::String> specifier) {
  std::string referrer_string;
  if (!referrer.IsEmpty()) {
    v8::Local<v8::Value> resource_name = referrer->GetResourceName();
    if (!resource_name.IsEmpty() && resource_name->IsString())
      referrer_string = toString(resource_name);
  }

  return Runtime::FromIsolate(context->GetIsolate())->GetModulator()->LoadModule(
    context, base::FilePath(referrer_string), toString(specifier));
}

RuntimeModulator::RuntimeModulator(v8::Isolate* isolate, const base::FilePath& root)
  : isolate_(isolate), root_(root) {}

RuntimeModulator::~RuntimeModulator() = default;

void RuntimeModulator::AddSyntheticModule(
    const std::string& name,
    std::unique_ptr<RuntimeModulator::SyntheticModule> module) {
  synthetic_modules_[name] = std::move(module);
}

v8::MaybeLocal<v8::Promise> RuntimeModulator::LoadModule(
  v8::Local<v8::Context> context,
  const base::FilePath& referrer,
  const std::string& specifier) {
  v8::MaybeLocal<v8::Promise::Resolver> maybe_resolver =
      v8::Promise::Resolver::New(context);
  v8::Local<v8::Promise::Resolver> resolver;
  if (maybe_resolver.ToLocal(&resolver)) {
    ResolveOrCreateModule(context, resolver, referrer, specifier);
    return resolver->GetPromise();
  }

  return v8::MaybeLocal<v8::Promise>();
}

v8::MaybeLocal<v8::Module> RuntimeModulator::GetModule(
    v8::Local<v8::Context> context,
    v8::Local<v8::Module> referrer,
    const std::string& specifier) {
  if (IsSyntheticModule(specifier))
    return GetSyntheticModule(specifier);

  // Find the |referrer| in the list of loaded modules, where it is expected to
  // be, and use it to resolve the path of the |specifier| that is to be loaded.
  for (const auto& pair : modules_) {
    if (pair.second != referrer)
      continue;

    base::FilePath path;
    if (!ResolveModulePath(pair.first, specifier, &path)) {
      LOG(ERROR) << "Unable to get the module for: " << specifier;
      return v8::MaybeLocal<v8::Module>();
    }

    return GetModule(path);
  }

  DCHECK(false) << "GetModule() calls should always succeed.";
  return v8::MaybeLocal<v8::Module>();
}

void RuntimeModulator::ClearCache(const std::string& relative_prefix) {
  fs::path root(root_.value());
  fs::path prefix(fs::absolute(fs::path(relative_prefix), root));

  const std::string prefix_str = prefix.string();

  std::vector<base::FilePath> to_remove;
  for (const auto& pair : modules_) {
    if (!pair.first.value().find(prefix_str))
      to_remove.push_back(pair.first);
  }

  for (const base::FilePath& path : to_remove)
    modules_.erase(path);
}

void RuntimeModulator::ResolveOrCreateModule(
  v8::Local<v8::Context> context,
  v8::Local<v8::Promise::Resolver> resolver,
  const base::FilePath& referrer,
  const std::string& specifier) {
  v8::TryCatch try_catch(context->GetIsolate());
  v8::Local<v8::Module> module;

  base::FilePath path;

  const bool is_synthetic = IsSyntheticModule(specifier);

  // Resolve the path of the module that is to be loaded. It is considered a
  // fatal error when we cannot resolve the path.
  if (!is_synthetic && !ResolveModulePath(referrer, specifier, &path)) {
    resolver->Reject(context, try_catch.Exception());
    return;
  }

  // (1) Reuse a previously loaded module if it exists.
  v8::MaybeLocal<v8::Module> existing_module =
      is_synthetic ? GetSyntheticModule(specifier)
                   : GetModule(path);

  if (!existing_module.ToLocal(&module)) {
    // (2) Attempt to create a new module for the given |path|.
    if (!CreateModule(context, path).ToLocal(&module)) {
      // (3) Reject the |resolver| since the |path| cannot be loaded.
      resolver->Reject(context, try_catch.Exception());
      return;
    }
  }

  // Instantiate the module for the current import.
  v8::MaybeLocal<v8::Value> maybe_module_result;
  if (module->InstantiateModule(context, ResolveModuleCallback).FromMaybe(false))
    maybe_module_result = module->Evaluate(context);

  v8::Local<v8::Value> module_result;
  if (!maybe_module_result.ToLocal(&module_result)) {
    resolver->Reject(context, try_catch.Exception()).ToChecked();
    return;
  }

  v8::Local<v8::Value> module_ns = module->GetModuleNamespace();
  resolver->Resolve(context, module_ns).ToChecked();
}

v8::MaybeLocal<v8::Module> RuntimeModulator::GetModule(const base::FilePath& path) {
  auto iter = modules_.find(path);
  if (iter == modules_.end())
    return v8::MaybeLocal<v8::Module>();

  return iter->second.Get(v8::Isolate::GetCurrent());
}

v8::MaybeLocal<v8::Module> RuntimeModulator::CreateModule(
    v8::Local<v8::Context> context,
    const base::FilePath& path) {
  v8::Local<v8::String> code;
  if (!ReadFile(path, &code)) {
    ThrowException("Unable to open the module for reading: " + path.value());
    return v8::MaybeLocal<v8::Module>();
  }

  v8::Isolate* isolate = context->GetIsolate();
  v8::ScriptOrigin origin(
      v8String(path.value()),
      v8::Local<v8::Integer>() /* resource_line_offset */,
      v8::Local<v8::Integer>() /* resource_column_offset */,
      v8::Local<v8::Boolean>() /* resource_is_shared_cross_origin */,
      v8::Local<v8::Integer>() /* script_id */,
      v8::Local<v8::Value>() /* source_map_url */,
      v8::Local<v8::Boolean>() /* resource_is_opaque */,
      v8::Local<v8::Boolean>() /* is_wasm */,
      v8::True(isolate) /* is_module */);

  v8::ScriptCompiler::Source source(code, origin);

  v8::Local<v8::Module> module;
  if (!v8::ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module))
    return v8::MaybeLocal<v8::Module>();

  DCHECK(!modules_.count(path));
  modules_.emplace(path, v8::Global<v8::Module>(isolate, module));

  for (int i = 0; i < module->GetModuleRequestsLength(); ++i) {
    const std::string specifier = toString(module->GetModuleRequest(i));
    const bool is_synthetic = IsSyntheticModule(specifier);

    ScopedExceptionAttribution attribution(
        path, module->GetModuleRequestLocation(i).GetLineNumber());

    // Special-case attempted module loads from HTTP. This is supported in many
    // other environments, but we've opted not to support this for now. Bail out
    // immediately with an exception.
    if (IsHTTP(specifier)) {
      ThrowException("Serving modules over HTTP(s) is not supported: " + specifier);
      return v8::Local<v8::Module>();
    }

    if (is_synthetic)
      continue;  // identity already verified

    base::FilePath request_path;
    if (!ResolveModulePath(path, specifier, &request_path))
      return v8::Local<v8::Module>();

    // Attempt to get the v8::Module if it's already been loaded.
    if (!GetModule(request_path).IsEmpty())
      continue;

    // Attempt to load the v8::Module given that that's not the case.
    if (!CreateModule(context, request_path).IsEmpty())
      continue;

    return v8::Local<v8::Module>();
  }

  return module;
}

bool RuntimeModulator::ResolveModulePath(
    const base::FilePath& referrer,
    const std::string& specifier,
    base::FilePath* path) const {
  fs::path root(root_.value());

  // (1) Attempt to resolve the |specifier| against the |referrer|.
  if (!referrer.empty()) {
    fs::path referrer_relative =
        fs::absolute(fs::path(specifier), fs::path(referrer.value()));
    if (FileExists(referrer_relative)) {
      *path = base::FilePath(referrer_relative.string());
      return true;
    }
  }

  // (2) Attempt to resolve the |specifier| against the |root_|.
  fs::path root_relative = fs::absolute(fs::path(specifier), root);
  if (FileExists(root_relative)) {
    *path = base::FilePath(root_relative.string());
    return true;
  }

  ThrowException("Unable to resolve import: " + specifier);
  return false;
}

bool RuntimeModulator::ReadFile(const base::FilePath& path,
                                v8::Local<v8::String>* contents) {
  std::ifstream handle(path.value().c_str());
  if (!handle.is_open() || handle.fail())
    return false;

  std::stringstream source_stream;
  std::copy(std::istreambuf_iterator<char>(handle),
            std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(source_stream));

  *contents = v8String(source_stream.str());
  return true;
}

v8::MaybeLocal<v8::Module> RuntimeModulator::GetSyntheticModule(const std::string& specifier) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  auto cached_iter = loaded_synthetic_modules_.find(specifier);
  if (cached_iter != loaded_synthetic_modules_.end())
    return cached_iter->second.Get(isolate);

  auto module_iter = synthetic_modules_.find(specifier);
  CHECK(module_iter != synthetic_modules_.end());

  auto* synthetic_module = module_iter->second.get();

  std::vector<v8::Local<v8::String>> export_names;
  for (const std::string& export_name : synthetic_module->GetExportNames())
    export_names.push_back(v8String(export_name));

  v8::Local<v8::Module> module =
      v8::Module::CreateSyntheticModule(isolate, v8String(specifier), export_names,
                                        RuntimeModulator::EvaluateSyntheticModule);

  loaded_synthetic_modules_.emplace(specifier, v8::Global<v8::Module>(isolate, module));
  return module;
}

class SyntheticModuleRegistrarModuleImpl : public RuntimeModulator::SyntheticModuleRegistrar {
 public:
  SyntheticModuleRegistrarModuleImpl(v8::Local<v8::Context> context, v8::Local<v8::Module> module)
      : context_(context), module_(module) {}

  ~SyntheticModuleRegistrarModuleImpl() override = default;

  // RuntimeModulator::SyntheticModuleRegistrar implementation:
  void RegisterExport(const std::string& name, v8::Local<v8::Value> value) {
    module_->SetSyntheticModuleExport(context_->GetIsolate(), v8String(name), value);
  }

 private:
  v8::Local<v8::Context> context_;
  v8::Local<v8::Module> module_;
};

// static
v8::MaybeLocal<v8::Value> RuntimeModulator::EvaluateSyntheticModule(v8::Local<v8::Context> context,
                                                                    v8::Local<v8::Module> module) {
  v8::Isolate* isolate = context->GetIsolate();

  RuntimeModulator* instance = Runtime::FromIsolate(isolate)->GetModulator();
  SyntheticModule* synthetic_module = nullptr;

  for (const auto& iter : instance->loaded_synthetic_modules_) {
    if (iter.second.Get(isolate)->GetIdentityHash() != module->GetIdentityHash())
      continue;

    auto module_iter = instance->synthetic_modules_.find(iter.first);
    CHECK(module_iter != instance->synthetic_modules_.end());

    synthetic_module = module_iter->second.get();
    break;
  }

  CHECK(synthetic_module);

  std::unique_ptr<SyntheticModuleRegistrar> registrar =
      std::make_unique<SyntheticModuleRegistrarModuleImpl>(context, module);

  synthetic_module->RegisterExports(context, registrar.get());

  return v8::Undefined(isolate);
}

bool RuntimeModulator::IsSyntheticModule(const std::string& specifier) const {
  return synthetic_modules_.find(specifier) != synthetic_modules_.end();
}

RuntimeModulator::SyntheticModule* RuntimeModulator::GetSyntheticModulePtr(const std::string& specifier) const {
  auto module_iter = synthetic_modules_.find(specifier);
  if (module_iter == synthetic_modules_.end())
    return nullptr;

  return module_iter->second.get();
}

}  // namespace bindings
