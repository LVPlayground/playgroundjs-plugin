// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_OBJECTS_CONSOLE_H_
#define PLAYGROUND_BINDINGS_OBJECTS_CONSOLE_H_

#include "bindings/global_object.h"

namespace bindings {

class Runtime;

// The Console object (and "console" instance) enable the script to log output to the console, which
// will also be represented in the SA-MP server log. The syntax follows browser implementations.
class Console : public GlobalObject {
 public:
  explicit Console(Runtime* runtime);
  ~Console();

  enum Severity {
    SEVERITY_INFO,
    SEVERITY_LOG,
    SEVERITY_WARN,
    SEVERITY_ERROR
  };

  // Implementation of |console.error|, |console.info|, |console.log| and |console.warn|, each of
  // which defer to Output with the appropriate severity level.
  static void Error(const v8::FunctionCallbackInfo<v8::Value>& arguments) { Output(SEVERITY_ERROR, arguments); }
  static void Info(const v8::FunctionCallbackInfo<v8::Value>& arguments) { Output(SEVERITY_INFO, arguments); }
  static void Log(const v8::FunctionCallbackInfo<v8::Value>& arguments) { Output(SEVERITY_LOG, arguments); }
  static void Warn(const v8::FunctionCallbackInfo<v8::Value>& arguments) { Output(SEVERITY_WARN, arguments); }

  // Outputs the arguments contained in |arguments|, each on its own line.
  static void Output(Severity severity, const v8::FunctionCallbackInfo<v8::Value>& arguments);

  // Outputs |argument| with |severity| as its severity. Different from the immediate function
  // callbacks, this has to be executed on the Console instance.
  void OutputArgument(Severity severity, v8::Local<v8::Value> value);

  // GlobalObject implementation.
  void InstallPrototype(v8::Local<v8::ObjectTemplate> global) override;
  void InstallObjects(v8::Local<v8::Object> global) override;
};

}

#endif  // PLAYGROUND_BINDINGS_OBJECTS_CONSOLE_H_
