// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <memory>
#include <set>
#include <stdint.h>

#include "base/macros.h"

namespace plugin {
class PluginController;
}

namespace bindings {
namespace streamer {

class StreamerWorker;

// Host interface for the Streamer. Owned by the Runtime, accessed by both the Runtime and the
// StreamerModule based on the operation that has to take place. Queues work for the StreamerWorker,
// which runs on a background thread for performance reasons.
class StreamerHost {
 public:
  StreamerHost(plugin::PluginController* plugin_controller,
               boost::asio::io_context& main_thread_io_context,
               boost::asio::io_context& background_thread_io_context);
  ~StreamerHost();

  // -----------------------------------------------------------------------------------------------

  // Creates a new streamer. Returns a globally unique ID for the streamer.
  uint32_t CreateStreamer(uint16_t max_visible, uint16_t max_distance);

  // Adds a new entity to the streamer with the given |x|, |y| and |z| coordinates.
  uint32_t Add(uint32_t streamer_id, float x, float y, float z);

  // Requests the streamer plane to be optimised. Useful after adding a lot of entries.
  void Optimise(uint32_t streamer_id);

  // Requests the streamer to stream. Invokes |callback| with visible entities when finished.
  bool Stream(uint32_t streamer_id, boost::function<void(std::set<uint32_t>)> callback);

  // Deletes the entity with the given |entity_id| from the given |streamer_id|.
  void Delete(uint32_t streamer_id, uint32_t entity_id);

  // Deletes the streamer that exists with the given |streamer_id|.
  void DeleteStreamer(uint32_t streamer_id);

  // -----------------------------------------------------------------------------------------------

  // Called for every frame on the server. The host will gather positions of all online players at
  // a particular cadence to be able to better understand positions.
  void OnFrame(double current_time);

  // Sets the set of tracked player IDs for which the streamer has to cater. This is entirely
  // controlled by JavaScript, which has knowledge of the connected players.
  void SetTrackedPlayers(std::set<uint16_t> tracked_players);

 private:
  // Calls the given |function| on the worker thread. All methods called on the StreamerWorker class
  // beyond the constructor must be called on that thread for data safety.
  void CallOnWorkerThread(boost::function<void()> function);

  // Utility function to get the position of the given |playerid|. The |position| pointer must point
  // to an array being able to hold at least three floating point values.
  void GetPlayerPosition(uint32_t playerid, float** position) const;

  // Utility function to get the interior Id of the given |playerid|.
  uint32_t GetPlayerInteriorId(uint32_t playerid) const;

  // Utility function to get the virtual world of the given |playerid|.
  uint32_t GetPlayerVirtualWorld(uint32_t playerid) const;

  plugin::PluginController* plugin_controller_;

  boost::asio::io_context& main_thread_io_context_;
  boost::asio::io_context& background_thread_io_context_;

  std::shared_ptr<StreamerWorker> worker_;

  std::set<uint32_t> active_streamer_ids_;
  uint32_t last_streamer_id_ = 0;
  uint32_t last_entity_id_ = 0;

  std::set<uint16_t> tracked_players_;
  bool tracked_players_invalidated_ = false;

  double last_update_time_;

  DISALLOW_COPY_AND_ASSIGN(StreamerHost);
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_
