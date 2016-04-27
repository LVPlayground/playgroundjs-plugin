// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground_controller.h"

#include "base/file_path.h"
#include "base/logging.h"
#include "base/time.h"
#include "bindings/event.h"
#include "bindings/global_scope.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"
#include "performance/scoped_trace.h"
#include "plugin/callback.h"
#include "plugin/plugin_controller.h"

namespace playground {

PlaygroundController::PlaygroundController(plugin::PluginController* plugin_controller)
    : plugin_controller_(plugin_controller),
      runtime_(bindings::Runtime::Create(this, plugin_controller)) {}

PlaygroundController::~PlaygroundController() {}

void PlaygroundController::OnCallbacksAvailable(const std::vector<plugin::Callback>& callbacks) {
  bindings::GlobalScope* global = runtime_->GetGlobalScope();

  // Create event interfaces for each of the |callbacks| and install them on the global scope. This
  // will make sure that the properties of the SA-MP events can be inspected by the script.
  for (const auto& callback : callbacks)
    global->RegisterEvent(callback.name, bindings::Event::Create(callback));
}

bool PlaygroundController::OnCallbackIntercepted(const std::string& callback,
                                                 const plugin::Arguments& arguments) {
  // Convert the |callback| name to the associated idiomatic JavaScript event type.
  const std::string& type = bindings::Event::ToEventType(callback);

  performance::ScopedTrace trace(performance::INTERCEPTED_CALLBACK_TOTAL, type);
  bindings::GlobalScope* global = runtime_->GetGlobalScope();

  // Bail out immediately if there are no listeners for this callback.
  if (!global->HasEventListeners(type))
    return false;

  v8::HandleScope handle_scope(runtime_->isolate());
  v8::Context::Scope context_scope(runtime_->context());

  bindings::Event* event = global->GetEvent(callback);
  DCHECK(event);

  return global->DispatchEvent(type, event->NewInstance(arguments));
}

// TODO: Clean up everything after here.

void PlaygroundController::OnGamemodeLoaded() {
  runtime_->Initialize();

  {
    performance::ScopedTrace trace(performance::LOAD_JAVASCRIPT_TRACE, "main.js");
    v8::HandleScope scope(runtime_->isolate());

    const bool result =
        runtime_->ExecuteFile(base::FilePath("main.js"),
                              bindings::Runtime::EXECUTION_TYPE_NORMAL,
                              nullptr /** result **/);

    if (!result)
      LOG(ERROR) << "Unable to load the main JavaScript file.";

    runtime_->GetGlobalScope()->Finalize();
  }
}

void PlaygroundController::OnServerFrame() {
  runtime_->OnFrame();
}

void PlaygroundController::OnScriptOutput(const std::string& message) {
  if (!message.length())
    return;

  plugin_controller_->Output(message);
}

void PlaygroundController::OnScriptError(const std::string& filename, size_t line_number, const std::string& message) {
  std::string output_message;
  output_message = "[" + filename + ":" + std::to_string(line_number) + "] " + message;

  plugin_controller_->Output(output_message.c_str());
}

void PlaygroundController::OnScriptTestsDone(unsigned int total_tests, unsigned int failed_tests) {
  plugin_controller_->DidRunTests(total_tests, failed_tests);
}

}  // namespace playground
