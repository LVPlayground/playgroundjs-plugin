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
#include "plugin/callback.h"
#include "plugin/plugin_controller.h"

namespace playground {

namespace {

// Event type of the "frame" event, which should be run for every server frame.
const std::string g_frame_event_type = "frame";

}  // namespace

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
    v8::HandleScope scope(runtime_->isolate());

    const bool result =
        runtime_->ExecuteFile(base::FilePath("main.js"),
                              bindings::Runtime::EXECUTION_TYPE_NORMAL,
                              nullptr /** result **/);

    if (!result)
      LOG(ERROR) << "Unable to load the main JavaScript file.";
  }
}

void PlaygroundController::OnGamemodeUnloaded() {
  
}

void PlaygroundController::OnServerFrame() {
  runtime_->OnFrame();

  bindings::GlobalScope* global = runtime_->GetGlobalScope();

  // Bail out immediately if there are no listeners for the frame callback.
  if (!global->HasEventListeners(g_frame_event_type))
    return;
  
  v8::HandleScope handle_scope(runtime_->isolate());
  v8::Context::Scope context_scope(runtime_->context());

  v8::Local<v8::Object> event = v8::Object::New(runtime_->isolate());

  // The "frame" event has a "now" property on the event object which refers to the monotonically
  // increasing time, allowing timers to be based on the implementation.
  event->Set(bindings::v8String("now"),
             v8::Number::New(runtime_->isolate(), base::monotonicallyIncreasingTime()));

  global->DispatchEvent(g_frame_event_type, event);
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
