// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_

#include <boost/asio.hpp>
#include <set>

#include "base/macros.h"

namespace bindings {
namespace streamer {

// Host interface for the Streamer. Owned by the Runtime, accessed by both the Runtime and the
// StreamerModule based on the operation that has to take place. Queues work for the StreamerWorker,
// which runs on a background thread for performance reasons.
class StreamerHost {
 public:
  StreamerHost(boost::asio::io_context& main_thread_io_context,
               boost::asio::io_context& background_thread_io_context);
  ~StreamerHost();

  // Called for every frame on the server. The host will gather positions of all online players at
  // a particular cadence to be able to better understand positions.
  void OnFrame(double current_time);

  // Sets the set of tracked player IDs for which the streamer has to cater. This is entirely
  // controlled by JavaScript, which has knowledge of the connected players.
  void SetTrackedPlayers(std::set<uint16_t> tracked_players);

 private:
  boost::asio::io_context& main_thread_io_context_;
  boost::asio::io_context& background_thread_io_context_;

  std::set<uint16_t> tracked_players_;

  DISALLOW_COPY_AND_ASSIGN(StreamerHost);
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_
