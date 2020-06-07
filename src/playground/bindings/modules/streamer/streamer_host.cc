// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer_host.h"

#include <boost/bind/bind.hpp>
#include <vector>

#include "base/logging.h"
#include "base/time.h"
#include "bindings/modules/streamer/streamer_update.h"
#include "bindings/modules/streamer/streamer_worker.h"
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
      worker_(std::make_shared<StreamerWorker>(main_thread_io_context)),
      last_update_time_(base::monotonicallyIncreasingTime()) {}

StreamerHost::~StreamerHost() = default;

// -----------------------------------------------------------------------------------------------

uint32_t StreamerHost::CreateStreamer(uint16_t max_visible, uint16_t max_distance) {
  active_streamer_ids_.insert(++last_streamer_id_);

  CallOnWorkerThread(boost::bind(&StreamerWorker::Initialize, worker_, last_streamer_id_,
                                 max_visible, max_distance));

  return last_streamer_id_;
}

uint32_t StreamerHost::Add(uint32_t streamer_id, float x, float y, float z) {
  CallOnWorkerThread(boost::bind(&StreamerWorker::Add, worker_, streamer_id, ++last_entity_id_,
                                 x, y, z));

  return last_entity_id_;
}

void StreamerHost::Delete(uint32_t streamer_id, uint32_t entity_id) {
  CallOnWorkerThread(boost::bind(&StreamerWorker::Delete, worker_, streamer_id, entity_id));
}

void StreamerHost::DeleteStreamer(uint32_t streamer_id) {
  if (active_streamer_ids_.find(streamer_id) == active_streamer_ids_.end()) {
    LOG(WARNING) << "Unable to delete streamer with invalid ID: " << streamer_id;
    return;
  }

  CallOnWorkerThread(boost::bind(&StreamerWorker::DeleteAll, worker_, streamer_id));

  active_streamer_ids_.erase(streamer_id);
}

// -----------------------------------------------------------------------------------------------

void StreamerHost::OnFrame(double current_time) {
  if ((current_time - last_update_time_) > kStreamerUpdateIntervalMs)
    return;

  last_update_time_ = current_time;

  std::vector<StreamerUpdate> updates;
  for (uint16_t playerid : tracked_players_) {
    StreamerUpdate update;

    GetPlayerPosition(playerid, (float**) &update.position);

    update.interior = GetPlayerInteriorId(playerid);
    update.virtual_world = GetPlayerVirtualWorld(playerid);

    updates.emplace_back(std::move(update));
  }

  if (updates.size() || tracked_players_invalidated_)
    CallOnWorkerThread(boost::bind(&StreamerWorker::Update, worker_, updates));

  tracked_players_invalidated_ = false;
}

void StreamerHost::SetTrackedPlayers(std::set<uint16_t> tracked_players) {
  tracked_players_ = std::move(tracked_players);
  tracked_players_invalidated_ = true;
}

// -----------------------------------------------------------------------------------------------

void StreamerHost::CallOnWorkerThread(boost::function<void()> function) {
  background_thread_io_context_.post(function);
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
