// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_SCRIPT_PROLOGUE_H_
#define PLAYGROUND_BINDINGS_SCRIPT_PROLOGUE_H_

namespace bindings {

// The script prologue is a piece of JavaScript that has to be executed before any file is loaded
// into the interpreter. This enables us to write the global require() function in JavaScript.
extern const char kScriptPrologue[];

// Prologue and epilogue JavaScript code to prepend and append to modules. This ensures that they
// execute with limited visibility and follow the CommonJS paradigms.
extern const char kModulePrologue[];
extern const char kModuleEpilogue[];

}  // namespace

#endif  // PLAYGROUND_BINDINGS_SCRIPT_PROLOGUE_H_
