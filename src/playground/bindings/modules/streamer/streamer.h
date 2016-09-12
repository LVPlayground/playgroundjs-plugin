// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_H_

#include "base/macros.h"

#include <iostream>
#include <boost/geometry/index/rtree.hpp>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace streamer {

// Implementation of the streamer. Takes commands from and delivers results to JavaScript through
// the bindings provided by the module code.
class Streamer {
public:
  Streamer(uint32_t max_visible, double stream_distance);
  virtual ~Streamer();

  // Adds the entity identified by |id| to the tree, with the given parameters.
  void Add(uint32_t id, double x, double y, double z);

  // Streams the entities stored in this streamer, returning the |max_visible| closest entities
  // to the given point that are within |stream_distance|.
  const std::vector<uint32_t>& Stream(uint32_t visible, double x, double y, double z) const;

  // Deletes the entity identified by |id| from the tree.
  void Delete(uint32_t id);

  // Deletes all entities from the tree.
  void Clear();

  // Returns the number of entities that have been added to the tree.
  size_t size() const { return entities_.size(); }

private:
  // Type representing a 3D point in the tree. Must be Boost indexable.
  using Point = boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian>;

  // Type representing a 3D box used to define a bounding box.
  using Box = boost::geometry::model::box<Point>;

  // Parameters of the tree that will be used to represent the streaming entities. We use an R* tree
  // with a maximum node count of sixteen. Insertion time is sacrificed for query time.
  using TreeValue = std::pair<Point, uint32_t>;
  using TreeType = boost::geometry::index::rstar<16>;
  using Tree = boost::geometry::index::rtree<TreeValue, TreeType>;

  // The maximum distance from the streaming point selected entities may be.
  double stream_distance_;

  // The tree that stores the entities created on the plane.
  Tree tree_;

  // Mapping of the created entities, by their id, to the point they represent.
  std::unordered_map<uint32_t, Point> entities_;

  // Vector that will be reused to return the results of a streaming operation.
  mutable std::vector<uint32_t> results_;

  DISALLOW_COPY_AND_ASSIGN(Streamer);
};

}  // namespace streamer

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_H_
