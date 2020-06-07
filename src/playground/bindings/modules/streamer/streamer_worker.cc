// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer_worker.h"

#include <boost/bind/bind.hpp>

#include "base/logging.h"
#include "bindings/modules/streamer/streamer.h"

namespace bindings {
namespace streamer {

StreamerWorker::StreamerWorker(boost::asio::io_context& main_thread_io_context)
    : main_thread_io_context_(main_thread_io_context) {}

StreamerWorker::~StreamerWorker() = default;

void StreamerWorker::Initialize(uint32_t streamer_id, uint16_t max_visible, uint16_t max_distance) {
  streamers_.insert({ streamer_id, std::make_unique<Streamer>(max_visible, max_distance) });
}

void StreamerWorker::Add(uint32_t streamer_id, uint32_t entity_id, float x, float y, float z) {
  auto iterator = streamers_.find(streamer_id);
  if (iterator != streamers_.end())
    iterator->second->Add(entity_id, x, y, z);
}

void StreamerWorker::Optimise(uint32_t streamer_id) {
  auto iterator = streamers_.find(streamer_id);
  if (iterator != streamers_.end())
    iterator->second->Optimise();
}

void StreamerWorker::Update(std::vector<StreamerUpdate> updates) {
  latest_update_ = std::move(updates);
}

void StreamerWorker::Stream(uint32_t streamer_id, boost::function<void(std::set<uint32_t>)> callback) {
  std::set<uint32_t> entities;

  auto iterator = streamers_.find(streamer_id);
  if (iterator != streamers_.end())
    entities = iterator->second->Stream(latest_update_);

  main_thread_io_context_.post(boost::bind(callback, std::move(entities)));
}

void StreamerWorker::Delete(uint32_t streamer_id, uint32_t entity_id) {
  auto iterator = streamers_.find(streamer_id);
  if (iterator != streamers_.end())
    iterator->second->Delete(entity_id);
}

void StreamerWorker::DeleteAll(uint32_t streamer_id) {
  streamers_.erase(streamer_id);
}

}  // namespace streamer
}  // namespace bindings
