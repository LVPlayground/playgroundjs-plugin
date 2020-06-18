// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/execute.h"

#include <boost/asio/io_context.hpp>

#include "base/logging.h"
#include "gtest/gtest.h"

namespace bindings {

TEST(Execute, EchoTest) {
  boost::asio::io_context context;

  Execute(context, "echo", { "hello world" },
    [](int exit_code, const std::string& output, const std::string& error) {
      LOG(INFO) << output;
    });

  context.run();
}

}  // namespace bindings
