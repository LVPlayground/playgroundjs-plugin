// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/runtime_modulator.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "base/logging.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"

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

v8::MaybeLocal<v8::Module> ResolveModuleCallback(
    v8::Local<v8::Context> context,
    v8::Local<v8::String> specifier,
    v8::Local<v8::Module> referrer) {
  std::string referrer_string;
  return Runtime::FromIsolate(context->GetIsolate())->GetModulator()->GetModule(
    context, referrer, toString(specifier));
}

}  // namespace

// static
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
    context, referrer_string, toString(specifier));
}

RuntimeModulator::RuntimeModulator(v8::Isolate* isolate, const base::FilePath& root)
  : isolate_(isolate), root_(root) {}

RuntimeModulator::~RuntimeModulator() = default;

v8::MaybeLocal<v8::Promise> RuntimeModulator::LoadModule(
  v8::Local<v8::Context> context,
  const std::string& referrer,
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

void RuntimeModulator::ResolveOrCreateModule(
  v8::Local<v8::Context> context,
  v8::Local<v8::Promise::Resolver> resolver,
  const std::string& referrer,
  const std::string& specifier) {
  v8::Local<v8::Module> module;
  base::FilePath path;

  v8::TryCatch try_catch(context->GetIsolate());
  try_catch.SetVerbose(true);

  // Resolve the path of the module that is to be loaded. It is considered a
  // fatal error when we cannot resolve the path.
  if (!ResolveModulePath(referrer, specifier, &path)) {
    resolver->Reject(context, try_catch.Exception());
    return;
  }

  // (1) Reuse a previously loaded module if it exists.
  v8::MaybeLocal<v8::Module> existing_module = GetModule(path);
  if (!existing_module.ToLocal(&module)) {
    // (2) Attempt to create a new module for the given |path|.
    if (!CreateModule(context, path).ToLocal(&module)) {
      // (3) Reject the |resolver| since the |path| cannot be loaded.
      DCHECK(try_catch.HasCaught());
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
  auto iter = modules_.find(path.value());
  if (iter == modules_.end())
    return v8::MaybeLocal<v8::Module>();

  return iter->second.Get(v8::Isolate::GetCurrent());
}

v8::MaybeLocal<v8::Module> RuntimeModulator::CreateModule(
    v8::Local<v8::Context> context,
    const base::FilePath& path) {
  v8::Local<v8::String> code;
  if (!ReadFile(path, &code)) {
    // TODO(Russell): We'll probably have to throw in ResolveModulePath()?
    ThrowException("Unable to load module code: " + path.value());
    return v8::MaybeLocal<v8::Module>();
  }

  v8::ScriptOrigin origin(
      v8String(path.value()),
      v8::Local<v8::Integer>() /* resource_line_offset */,
      v8::Local<v8::Integer>() /* resource_column_offset */,
      v8::Local<v8::Boolean>() /* resource_is_shared_cross_origin */,
      v8::Local<v8::Integer>() /* script_id */,
      v8::Local<v8::Value>() /* source_map_url */,
      v8::Local<v8::Boolean>() /* resource_is_opaque */,
      v8::Local<v8::Boolean>() /* is_wasm */,
      v8::True(context->GetIsolate()) /* is_module */);

  v8::ScriptCompiler::Source source(code, origin);

  v8::Local<v8::Module> module;
  if (!v8::ScriptCompiler::CompileModule(context->GetIsolate(), &source).ToLocal(&module))
    return v8::MaybeLocal<v8::Module>();

  DCHECK(!modules_.count(path.value()));
  modules_.emplace(path.value(), v8::Global<v8::Module>(context->GetIsolate(), module));

  for (int i = 0; i < module->GetModuleRequestsLength(); ++i) {
    const std::string specifier = toString(module->GetModuleRequest(i));

    // Special-case attempted module loads from HTTP. This is supported in many
    // other environments, but we've opted not to support this for now. Bail out
    // immediately with an exception.
    if (IsHTTP(specifier)) {
      ThrowException("Serving modules over HTTP(s) is not supported: " + specifier);
      return v8::Local<v8::Module>();
    }

    base::FilePath request_path;
    if (!ResolveModulePath(path.value(), specifier, &request_path))
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
    const std::string& referrer,
    const std::string& specifier,
    base::FilePath* path) const {
  LOG(INFO) << "Resolve [" << referrer << "][" << specifier << "]";

  // TODO(Russell): Support relative paths?
  *path = root_.Append(specifier);
  return true;
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

}  // namespace bindings
