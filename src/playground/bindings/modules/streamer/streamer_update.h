// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_UPDATE_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_UPDATE_H_

#include <stdint.h>

namespace bindings {
namespace streamer {

struct StreamerUpdate {
  StreamerUpdate() : playerid(0), position{ 0, 0, 0 }, interior(0), virtual_world(0) {}

  StreamerUpdate(StreamerUpdate&&) = default;
  StreamerUpdate(const StreamerUpdate&) = default;

  uint16_t playerid;

  float position[3];

  uint32_t interior;
  uint32_t virtual_world;
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_UPDATE_H_
