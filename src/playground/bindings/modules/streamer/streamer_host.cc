// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer_host.h"

namespace bindings {
namespace streamer {

StreamerHost::StreamerHost(boost::asio::io_context& main_thread_io_context,
                           boost::asio::io_context& background_thread_io_context)
    : main_thread_io_context_(main_thread_io_context),
      background_thread_io_context_(background_thread_io_context) {}

StreamerHost::~StreamerHost() = default;

void StreamerHost::OnFrame(double current_time) {}

void StreamerHost::SetTrackedPlayers(std::set<uint16_t> tracked_players) {
  tracked_players_ = std::move(tracked_players);
}

}  // namespace streamer
}  // namespace bindings
