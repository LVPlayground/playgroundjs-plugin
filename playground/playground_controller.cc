// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground_controller.h"

#include <algorithm>

#include "base/file_path.h"
#include "base/logging.h"
#include "base/string_piece.h"
#include "bindings/global_scope.h"
#include "bindings/runtime.h"
#include "bindings/runtime_options.h"
#include "bindings/script_loader.h"
#include "plugin/arguments.h"
#include "plugin/callback.h"
#include "plugin/plugin_controller.h"

namespace playground {

namespace {

// Directory in which the JavaScript code will reside.
const char kJavaScriptDirectory[] = "javascript";

// Filename of the main JavaScript file to begin executing.
const char kJavaScriptFile[] = "playground.js";

// Converts |callback_name| to a JavaScript event-target name.
std::string toEventName(base::StringPiece callback_name) {
  if (!callback_name.starts_with("On"))
    return callback_name.as_string();

  callback_name.remove_prefix(2 /* On */);

  std::string event_name = callback_name.as_string();
  std::transform(event_name.begin(), event_name.end(), event_name.begin(), ::tolower);

  return event_name;
}

}  // namespace

PlaygroundController::PlaygroundController(plugin::PluginController* plugin_controller)
    : plugin_controller_(plugin_controller) {}

PlaygroundController::~PlaygroundController() {
  runtime_->global_scope()->Dispose();
}

bool PlaygroundController::CreateRuntime() {
  bindings::RuntimeOptions options;
  options.strict_mode = true;

  // TODO: We may want to parse the server's configuration file to make the two constants
  // (kJavaScriptDirectory and kJavaScriptFile) configurable.

  runtime_.reset(new bindings::Runtime(options, this));

  base::FilePath script_directory = base::FilePath::CurrentDirectory();
  script_directory = script_directory.Append(kJavaScriptDirectory);

  // Initialize the script loading interface, which will be responsible for loading the actual
  // scripts into the engine. It will also expose functions to do so from JavaScript.
  script_loader_.reset(new bindings::ScriptLoader(runtime_.get(), script_directory));

  return true;
}

void PlaygroundController::OnCallbacksAvailable(const std::vector<plugin::Callback>& callbacks) {
  if (!CreateRuntime()) {
    LOG(FATAL) << "Unable to initialize the JavaScript runtime.";
    return;
  }

  bindings::GlobalScope* global_scope = runtime_->global_scope();
  for (const auto& callback : callbacks)
    global_scope->CreateInterfaceForCallback(callback);

  runtime_->Initialize();
  if (!script_loader_->Load(base::FilePath(kJavaScriptFile)))
    LOG(ERROR) << "Unable to load the main JavaScript file.";
}

bool PlaygroundController::OnCallbackIntercepted(const plugin::Callback& callback,
                                                 const plugin::Arguments& arguments) {
  v8::HandleScope handle_scope(runtime_->isolate());
  v8::Context::Scope context_scope(runtime_->context());

  bindings::GlobalScope* global_scope = runtime_->global_scope();

  const std::string event_name = toEventName(callback.name);
  if (!global_scope->hasEventListeners(event_name))
    return false;

  v8::Local<v8::Object> object = global_scope->CreateEventForCallback(callback, arguments);
  if (object.IsEmpty())
    return false;

  return global_scope->triggerEvent(event_name, object);
}

void PlaygroundController::OnGamemodeLoaded() {

}

void PlaygroundController::OnGamemodeUnloaded() {
  
}

void PlaygroundController::OnServerFrame() {

}

void PlaygroundController::OnScriptOutput(const std::string& message) {
  plugin_controller_->Output(message.c_str());
}

void PlaygroundController::OnScriptError(const std::string& filename, size_t line_number, const std::string& message) {
  std::string output_message;
  output_message = "[" + filename + ":" + std::to_string(line_number) + "] " + message;

  plugin_controller_->Output(output_message.c_str());
}

}  // namespace playground
