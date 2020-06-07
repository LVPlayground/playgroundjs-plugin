// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer_worker.h"

#include "base/logging.h"

namespace bindings {
namespace streamer {

StreamerWorker::StreamerWorker(boost::asio::io_context& main_thread_io_context)
    : main_thread_io_context_(main_thread_io_context) {}

StreamerWorker::~StreamerWorker() = default;

void StreamerWorker::Initialize(uint32_t streamer_id, uint16_t max_visible, uint16_t max_distance) {
  LOG(INFO) << __FUNCTION__;
}

void StreamerWorker::Add(uint32_t streamer_id, uint32_t entity_id, float x, float y, float z) {
  LOG(INFO) << __FUNCTION__;
}

void StreamerWorker::Update(std::vector<StreamerUpdate> updates) {
  LOG(INFO) << __FUNCTION__;
}

void StreamerWorker::Stream(uint32_t streamer_id, boost::function<void(std::set<uint32_t>)> callback) {
  LOG(INFO) << __FUNCTION__;
}

void StreamerWorker::Delete(uint32_t streamer_id, uint32_t entity_id) {
  LOG(INFO) << __FUNCTION__;
}

void StreamerWorker::DeleteAll(uint32_t streamer_id) {
  LOG(INFO) << __FUNCTION__;
}

void StreamerWorker::CallOnMainThread(boost::function<void()> function) {
  main_thread_io_context_.post(function);
}

}  // namespace streamer
}  // namespace bindings
