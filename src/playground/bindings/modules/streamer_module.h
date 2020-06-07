// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_MODULE_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_MODULE_H_

#include <include/v8.h>

namespace bindings {

// The Streamer module provides the primitive necessary for providing streaming of a specific sort
// of entity to a player. The interface is deliberately abstracted away from the sort of entity
// it streams, it is the responsibility of JavaScript to provide the additional functionality.
//
// [Constructor(number maxVisible, number streamingDistance = 300)]
// interface Streamer {
//     static setTrackedPlayers(Set playerIds);
//
//     number add(number x, number y, number z);
//     void optimise();
//     void delete(number entityId)
//
//     Promise<sequence<number>> stream();
// };
//
// The Streamer interface should only rarely be used directly. Instead, use the slightly higher-
// level implementations available in //features/streamer/.
class StreamerModule {
public:
  StreamerModule();
  ~StreamerModule();

  // Called when the prototype for the global object is being constructed. 
  void InstallPrototypes(v8::Local<v8::ObjectTemplate> global);
};

}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_MODULE_H_
