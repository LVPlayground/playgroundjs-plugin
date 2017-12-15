// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/bindings/modules/streamer/streamer.h"

#include "base/logging.h"

#include <boost/function_output_iterator.hpp>
#include <boost/geometry/geometry.hpp>
#include <iterator>

namespace streamer {
namespace {

// Iterator functor that inserts the integral Id of an entity in the output container.
class EntityIdBackInserter {
public:
  explicit EntityIdBackInserter(std::vector<uint32_t>* container)
      : container_(container) {}

  template <typename Value>
  void operator()(Value const& pair) {
    container_->push_back(pair.second);
  }

private:
  std::vector<uint32_t>* container_;
};

}  // namespace

Streamer::Streamer(uint32_t max_visible, double stream_distance)
    : stream_distance_(stream_distance) {
  results_.reserve(max_visible);
}

Streamer::~Streamer() {}

void Streamer::Add(uint32_t id, double x, double y, double z) {
  if (entities_.count(id)) {
    LOG(WARNING) << "An entry for Id #" << id << " already exists in the tree. Replacing.";
    Delete(id);
  }

  Point position(x, y, z);

  // Add the entity having |id| to the tree at |position|.
  tree_.insert(std::make_pair(position, id));

  // Associate the |position| with the |id|, so that it can easily be deleted.
  entities_.insert(std::make_pair(id, position));
}

void Streamer::Optimise() {
  Tree repackaged_tree(tree_.begin(), tree_.end());
  tree_.swap(repackaged_tree);
}

const std::vector<uint32_t>& Streamer::Stream(uint32_t visible, double x, double y, double z) const {
  results_.clear();

  EntityIdBackInserter inserter(&results_);

  Point point(x, y, z);

  tree_.query(boost::geometry::index::nearest(point, visible) &&
              boost::geometry::index::satisfies([&](const TreeValue& value) {
                  return boost::geometry::distance(value.first, point) < stream_distance_;
              }),
              boost::make_function_output_iterator(inserter));

  return results_;
}

void Streamer::Delete(uint32_t id) {
  auto iter = entities_.find(id);
  if (iter == entities_.end())
    return;

  tree_.remove(std::make_pair(iter->second, id));
  entities_.erase(iter);
}

void Streamer::Clear() {
  tree_.clear();
  entities_.clear();
}

}  // namespace streamer
