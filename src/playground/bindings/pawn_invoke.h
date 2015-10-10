// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_PAWN_INVOKE_H_
#define PLAYGROUND_BINDINGS_PAWN_INVOKE_H_

#include <memory>

#include <include/v8.h>

namespace plugin {
class PluginController;
}

namespace bindings {

// Implementation of the pawnInvoke() function on the JavaScript engine's global scope. This
// function makes it possible to call SA-MP functions from JavaScript, allowing interaction with the
// rest of the server, and is therefore critical to the plugin.
//
// The signature of the function is as follows:
//     any pawnInvoke(string name[, string signature[, ...]]);
//
// The function |name| must always be passed, and it must be a non-zero length string. It indicates
// the name of the SA-MP native that should be invoked, for example "GetMaxPlayers".
//
// The |signature| defines the signature of the native function. The syntax of the signature comes
// down to the to the following:
//
//     signature       =  (argument_types)*
//     argument_types  =  [fFiIsS]
//
//         'f' - float
//         'F' - float reference (will be returned)
//         'i' - integer
//         'I' - integer reference (will be returned)
//         's' - string
//         'S' - string reference (will be returned)
//
// Validation of the arguments will be done in a strict matter. Arguments of types [fis] must be
// present in the arguments following the |signature|. Note that it is not necessary to specify
// string length when using the [S] string reference argument from JavaScript.
//
// Finally, in the current implementation there is a limitation that no non-reference arguments
// may follow reference arguments. This could be optimized, but simplifies the initial version.
//
// Some examples of Pawn native functions with their mapping to the |signature| syntax:
//
//     native GetMaxPlayers();                          ''
//     native IsPlayerConnected(playerid);              'i'
//     native GetPlayerName(playerid, name[], len)      'iS'
//     native SetPlayerName(playerid, name[])           'is'
//     native GetPlayerPos(playerid, &Float: x, ...)    'iFFF'
//     native SetPlayerPos(playerid, Float: x, ...)     'ifff'
//
// TODO: Functions with non-integral return types need a way to indicate this in the signature,
// for example functions returning a Float or a bool.
class PawnInvoke {
 public:
  explicit PawnInvoke(plugin::PluginController* plugin_controller);
  ~PawnInvoke();

  // Invokes the Pawn function indicated by |arguments|, returning the resulting value(s). See
  // the class-level documentation about the inner workings of this method.
  v8::Local<v8::Value> Call(const v8::FunctionCallbackInfo<v8::Value>& arguments);

 private:
  // Maximum number of arguments and return values supported in a Pawn call.
  static const size_t kMaxArgumentCount = 24;

  struct StaticBuffer;

  // Parses the |signature| and stores the resulting types in |static_buffer_|. The number of
  // argument and return values will be stored in their respective out-arguments.
  bool ParseSignature(v8::Local<v8::Value> signature,
                      size_t* argument_count, size_t* return_count);

  // Memory allocated to provide static buffers for the invocation. We do this to prevent
  // having to make too many allocations each time a Pawn native function is invoked.
  std::unique_ptr<StaticBuffer> static_buffer_;

  // Instance of the plugin controller that the Pawn functions will be executed on.
  plugin::PluginController* plugin_controller_;
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_PAWN_INVOKE_H_
