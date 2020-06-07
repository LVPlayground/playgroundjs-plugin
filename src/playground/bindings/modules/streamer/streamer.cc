// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/streamer/streamer.h"

#include "base/logging.h"

namespace bindings {
namespace streamer {

Streamer::Streamer(uint16_t max_visible, uint16_t max_distance)
    : max_visible_(max_visible),
      max_distance_(max_distance) {}

Streamer::~Streamer() = default;

void Streamer::Add(uint32_t entity_id, float x, float y, float z) {
  LOG(INFO) << __FUNCTION__;
}

void Streamer::Optimise() {
  LOG(INFO) << __FUNCTION__;
}

std::set<uint32_t> Streamer::Stream(const std::vector<StreamerUpdate>& updates) {
  LOG(INFO) << __FUNCTION__;

  return std::set<uint32_t>();
}

void Streamer::Delete(uint32_t entity_id) {
  LOG(INFO) << __FUNCTION__;
}

}  // namespace streamer
}  // namespace bindings
