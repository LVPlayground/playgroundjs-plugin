// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/streamer/streamer.h"

#include <boost/geometry/geometry.hpp>

#include "base/logging.h"

namespace bindings {
namespace streamer {

Streamer::Streamer(uint16_t max_visible, uint16_t max_distance)
    : max_visible_(max_visible),
      max_distance_(max_distance) {}

Streamer::~Streamer() = default;

void Streamer::Add(uint32_t entity_id, float x, float y, float z) {
  Point position(x, y, z);

  entities_.insert({ entity_id, position });
  tree_.insert({ position, entity_id });
}

void Streamer::Optimise() {
  LOG(INFO) << __FUNCTION__;
}

std::set<uint32_t> Streamer::Stream(const std::vector<StreamerUpdate>& updates) {
  LOG(INFO) << __FUNCTION__;

  return std::set<uint32_t>();
}

void Streamer::Delete(uint32_t entity_id) {
  auto iterator = entities_.find(entity_id);
  if (iterator == entities_.end())
    return;

  tree_.remove({ iterator->second, entity_id });

  entities_.erase(iterator);
}

uint32_t Streamer::size() const {
  return tree_.size();
}

}  // namespace streamer
}  // namespace bindings
