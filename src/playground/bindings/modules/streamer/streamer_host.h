// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_
#define PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_

#include <boost/asio.hpp>

#include "base/macros.h"

namespace bindings {
namespace streamer {

class StreamerHost {
 public:
  StreamerHost(boost::asio::io_context& main_thread_io_context,
               boost::asio::io_context& background_thread_io_context);
  ~StreamerHost();

  // Called for every frame on the server. The host will gather positions of all online players at
  // a particular cadence to be able to better understand positions.
  void OnFrame(double current_time);

 private:
  boost::asio::io_context& main_thread_io_context_;
  boost::asio::io_context& background_thread_io_context_;

  DISALLOW_COPY_AND_ASSIGN(StreamerHost);
};

}  // namespace streamer
}  // namespace bindings

#endif  // PLAYGROUND_BINDINGS_MODULES_STREAMER_STREAMER_HOST_H_
