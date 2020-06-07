// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer_worker.h"

namespace bindings {
namespace streamer {

StreamerWorker::StreamerWorker(boost::asio::io_context& main_thread_io_context)
    : main_thread_io_context_(main_thread_io_context) {}

StreamerWorker::~StreamerWorker() = default;

void StreamerWorker::CallOnMainThread(boost::function<void()> function) {
  main_thread_io_context_.post(function);
}

}  // namespace streamer
}  // namespace bindings
