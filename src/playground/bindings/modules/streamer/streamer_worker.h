// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_WORKER_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_WORKER_H_

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <vector>

#include "base/macros.h"
#include "bindings/modules/streamer/streamer_update.h"

namespace bindings {
namespace streamer {

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

  // Called when there are |updates| in regards the the to-be-considered players, their positions,
  // interior Ids and virtual worlds. Will be stored for the next streamer update.
  void Update(std::vector<StreamerUpdate> updates);

  // Deletes the entity with the given |entity_id| from the given |streamer_id|.
  void Delete(uint32_t streamer_id, uint32_t entity_id);

  // Deletes all data associated with the given |streamer_id|.
  void DeleteAll(uint32_t streamer_id);

 private:
  // Calls the given |function| on the main thread. All responses and callbacks must be invoked on
  // the main thread as that's where V8 and the rest of the PlaygroundJS plugin code lives.
  void CallOnMainThread(boost::function<void()> function);

  boost::asio::io_context& main_thread_io_context_;

  DISALLOW_COPY_AND_ASSIGN(StreamerWorker);
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_WORKER_H_
