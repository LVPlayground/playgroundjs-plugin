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
  Point position(x, y);

  entities_.insert({ entity_id, position });
  tree_.insert({ position, entity_id });
}

void Streamer::Optimise() {
  Tree optimised_tree(tree_.begin(), tree_.end());
  tree_.swap(optimised_tree);
}

std::set<uint32_t> Streamer::Stream(const std::vector<StreamerUpdate>& updates) {
  std::vector<Tree::const_query_iterator> query_iterators;
  query_iterators.reserve(updates.size());

  uint32_t max_per_player = max_visible_ / updates.size();

  for (const StreamerUpdate& update : updates) {
    if (update.interior != 0 || update.virtual_world != 0)
      continue;
    
    Point position(update.position[0], update.position[1]);
    Box box(Point(update.position[0] - max_distance_, update.position[1] - max_distance_),
            Point(update.position[0] + max_distance_, update.position[1] + max_distance_));

    uint32_t max_visible = std::max(max_per_player * 2, 100u);

    query_iterators.push_back(
        tree_.qbegin(boost::geometry::index::within(box) &&
                     boost::geometry::index::nearest(position, max_visible)));
  }

  std::set<uint32_t> entities;

  // Add entities to the |entities| set in iterations. We begin by ensuring that the player's share
  // of the maximum visible entities is represented. After that we continue iterating over each of
  // the query iterators, adding similar size batches, until we either reach the maximum number of
  // entities on the server, or all available entities for the given players will be created.
  while (entities.size() < max_visible_ && query_iterators.size()) {
    max_per_player = std::max((max_visible_ - entities.size()) / updates.size(), 2u);

    for (auto iter = query_iterators.begin(); iter != query_iterators.end();) {
      if (entities.size() == max_visible_)
        break;

      auto& query_iterator = *iter;

      uint32_t batch_size = 0;
      for (; query_iterator != tree_.qend() && batch_size < max_per_player; ++query_iterator) {
        entities.insert(query_iterator->second);
        if (entities.size() == max_visible_)
          break;

        ++batch_size;
      }

      if (batch_size != max_per_player)
        iter = query_iterators.erase(iter);
      else
        iter++;
    }
  }

  return entities;
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
