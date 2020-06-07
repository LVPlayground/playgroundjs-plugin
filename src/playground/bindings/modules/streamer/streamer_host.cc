// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer_host.h"

#include "base/logging.h"
#include "base/time.h"
#include "plugin/plugin_controller.h"

namespace bindings {
namespace streamer {
namespace {

// Interval at which the streamer will update player positioning information.
const double kStreamerUpdateIntervalMs = 1000;

}  // namespace

StreamerHost::StreamerHost(plugin::PluginController* plugin_controller,
                           boost::asio::io_context& main_thread_io_context,
                           boost::asio::io_context& background_thread_io_context)
    : plugin_controller_(plugin_controller),
      main_thread_io_context_(main_thread_io_context),
      background_thread_io_context_(background_thread_io_context),
      last_update_time_(base::monotonicallyIncreasingTime()) {}

StreamerHost::~StreamerHost() = default;

void StreamerHost::OnFrame(double current_time) {
  if ((current_time - last_update_time_) < kStreamerUpdateIntervalMs)
    return;

  last_update_time_ = current_time;
}

void StreamerHost::SetTrackedPlayers(std::set<uint16_t> tracked_players) {
  tracked_players_ = std::move(tracked_players);
}

void StreamerHost::GetPlayerPosition(uint32_t playerid, float** position) const {
  void* arguments[4] = { &playerid, &position[0], &position[1], &position[2] };
  plugin_controller_->CallFunction("GetPlayerPos", "irrr", (void**) &arguments);
}

uint32_t StreamerHost::GetPlayerInteriorId(uint32_t playerid) const {
  void* arguments[1] = { &playerid };
  return static_cast<uint32_t>(
      plugin_controller_->CallFunction("GetPlayerInterior", "i", (void**) &arguments));
}

uint32_t StreamerHost::GetPlayerVirtualWorld(uint32_t playerid) const {
  void* arguments[1] = { &playerid };
  return static_cast<uint32_t>(
      plugin_controller_->CallFunction("GetPlayerVirtualWorld", "i", (void**) &arguments));
}

}  // namespace streamer
}  // namespace bindings
