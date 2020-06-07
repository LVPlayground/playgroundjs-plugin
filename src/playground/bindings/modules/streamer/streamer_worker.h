// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_WORKER_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_WORKER_H_

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "bindings/modules/streamer/streamer_update.h"

namespace bindings {
namespace streamer {

class Streamer;

// Worker for the streamer system. Runs on a background thread. Receives several signals from the
// host, particularly in regards to data manipulation. Data updates can be requested asynchronously.
class StreamerWorker {
 public:
  explicit StreamerWorker(boost::asio::io_context& main_thread_io_context);
  ~StreamerWorker();

  // Initializes a plane and general state for a new streamer with the given |streamer_id|.
  void Initialize(uint32_t streamer_id, uint16_t max_visible, uint16_t max_distance);

  // Adds a new entity to the streamer with the given |x|, |y| and |z| coordinates.
  void Add(uint32_t streamer_id, uint32_t entity_id, float x, float y, float z);

  // Requests the streamer plane to be optimised. Useful after adding a lot of entries.
  void Optimise(uint32_t streamer_id);

  // Called when there are |updates| in regards the the to-be-considered players, their positions,
  // interior Ids and virtual worlds. Will be stored for the next streamer update.
  void Update(std::vector<StreamerUpdate> updates);

  // Requests the streamer with the given |streamer_id| to stream. Will call |callback| when the
  // streaming calculations have finished.
  void Stream(uint32_t streamer_id, boost::function<void(std::set<uint32_t>)> callback);

  // Deletes the entity with the given |entity_id| from the given |streamer_id|.
  void Delete(uint32_t streamer_id, uint32_t entity_id);

  // Deletes all data associated with the given |streamer_id|.
  void DeleteAll(uint32_t streamer_id);

 private:
  boost::asio::io_context& main_thread_io_context_;

  std::vector<StreamerUpdate> latest_update_;
  std::unordered_map<uint32_t, std::unique_ptr<Streamer>> streamers_;

  DISALLOW_COPY_AND_ASSIGN(StreamerWorker);
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_WORKER_H_
