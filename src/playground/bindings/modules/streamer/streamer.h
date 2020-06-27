// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_H_

#include <boost/geometry/index/rtree.hpp>
#include <set>
#include <stdint.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "bindings/modules/streamer/streamer_update.h"

namespace bindings {
namespace streamer {

// Encapsulates an individual streamer. Fully lives on the background thread, and does not have
// to worry about thread safety at all.
class Streamer {
public:
  Streamer(uint16_t max_visible, uint16_t max_distance);
  ~Streamer();

  // Adds the given |entity_id| at the given |x|, |y|, |z| coordinates to this streamer.
  void Add(uint32_t entity_id, float x, float y, float z);

  // Optimises the streaming plane by request of the JavaScript code.
  void Optimise();

  // Streams all entities part of this streamer given the |updates|. Returns a set with all the
  // entity IDs that should be present in the world.
  std::set<uint32_t> Stream(const std::vector<StreamerUpdate>& updates);

  // Deletes the entity identified by the given |entity_id| from this streamer.
  void Delete(uint32_t entity_id);

  // Returns the number of entries that have been added to this streamer.
  uint32_t size() const;

 private:
  using Point = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
  using Box = boost::geometry::model::box<Point>;

  using TreeValue = std::pair<Point, uint32_t>;
  using TreeType = boost::geometry::index::rstar<32, 16>;
  using Tree = boost::geometry::index::rtree<TreeValue, TreeType>;

  int64_t instance_id_;

  uint16_t max_visible_;
  uint16_t max_distance_;

  std::unordered_map<uint32_t, Point> entities_;

  Tree tree_;

  DISALLOW_COPY_AND_ASSIGN(Streamer);
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_H_
