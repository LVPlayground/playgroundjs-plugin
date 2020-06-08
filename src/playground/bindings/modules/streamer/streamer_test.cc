// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer.h"

#include "gtest/gtest.h"

namespace bindings {
namespace streamer {

class StreamerTest : public ::testing::Test {
 public:
  StreamerTest() = default;
  ~StreamerTest() = default;
};

TEST_F(StreamerTest, AddOptimiseDelete) {
  const uint32_t kEntityId = 1337;

  Streamer streamer(/* max_visible= */ 100, /* max_distance= */ 300);
  EXPECT_EQ(streamer.size(), 0);

  streamer.Add(kEntityId, 100, 200, 300);
  EXPECT_EQ(streamer.size(), 1);

  streamer.Optimise();
  EXPECT_EQ(streamer.size(), 1);

  streamer.Delete(kEntityId);
  EXPECT_EQ(streamer.size(), 0);
}

}  // namespace streamer
}  // namespace bindings
