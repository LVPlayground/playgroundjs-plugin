// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer.h"

#include <random>

#include "base/logging.h"
#include "base/time.h"
#include "gtest/gtest.h"

namespace bindings {
namespace streamer {

class StreamerTest : public ::testing::Test {
 public:
  StreamerTest()
      : distribution_(-3000, 3000),
        height_distribution_(-100, 200) {}

  ~StreamerTest() = default;

 protected:
  inline float RandomX() { return distribution_(random_generator_); }
  inline float RandomY() { return distribution_(random_generator_); }
  inline float RandomZ() { return height_distribution_(random_generator_); }

 private:
  std::default_random_engine random_generator_;

  std::uniform_real_distribution<float> distribution_;
  std::uniform_real_distribution<float> height_distribution_;
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

TEST_F(StreamerTest, AllResultsStreamedIn) {
  Streamer streamer(/* max_visible= */ 100, /* max_distance= */ 10000);
  for (uint32_t entity_id = 1; entity_id <= 50; ++entity_id)
    streamer.Add(entity_id, RandomX(), RandomY(), RandomZ());
  
  ASSERT_EQ(streamer.size(), 50);

  std::vector<StreamerUpdate> updates;
  for (uint32_t playerid = 0; playerid < 3; ++playerid) {
    StreamerUpdate update;
    update.playerid = playerid;

    updates.push_back(std::move(update));
  }
  
  std::set<uint32_t> results = streamer.Stream(updates);
  EXPECT_EQ(results.size(), 50);
}

TEST_F(StreamerTest, SelectionPerPlayer) {
  Streamer streamer(/* max_visible= */ 1000, /* max_distance= */ 10000);
  for (uint32_t entity_id = 1; entity_id <= 5000; ++entity_id)
    streamer.Add(entity_id, RandomX(), RandomY(), RandomZ());

  ASSERT_EQ(streamer.size(), 5000);

  std::vector<StreamerUpdate> updates;
  for (uint32_t playerid = 0; playerid < 50; ++playerid) {
    StreamerUpdate update;
    update.playerid = playerid;
    update.position[0] = RandomX();
    update.position[1] = RandomY();
    update.position[2] = RandomZ();

    updates.push_back(std::move(update));
  }

  std::set<uint32_t> results = streamer.Stream(updates);
  EXPECT_NEAR(results.size(), 1000, 5);  // account for possible overlap
}

TEST_F(StreamerTest, SelectionPerPlayerWithOverlap) {
  Streamer streamer(/* max_visible= */ 100, /* max_distance= */ 150);
  for (uint32_t entity_id = 1; entity_id <= 100; ++entity_id)
    streamer.Add(entity_id, -50.0f + entity_id, -50.0f + entity_id, RandomZ());

  for (uint32_t entity_id = 101; entity_id <= 200; ++entity_id)
    streamer.Add(entity_id, 1000.0f + entity_id, 1000.0f + entity_id, RandomZ());

  std::vector<StreamerUpdate> updates;
  for (uint32_t playerid = 1; playerid <= 4; ++playerid) {
    StreamerUpdate update;
    update.playerid = playerid;
    update.position[0] = -50.0f + playerid * 25.0f;
    update.position[1] = -50.0f + playerid * 25.0f;
    update.position[2] = RandomZ();

    updates.push_back(std::move(update));
  }

  StreamerUpdate update;
  update.playerid = 0;
  update.position[0] = 1150.0;
  update.position[1] = 1150.0;
  update.position[2] = RandomZ();

  updates.push_back(std::move(update));

  std::set<uint32_t> results = streamer.Stream(updates);

  uint32_t center_count = 0;
  uint32_t offset_count = 0;

  for (uint32_t entity_id : results) {
    if (entity_id <= 100)
      ++center_count;
    else
      ++offset_count;
  }

  EXPECT_GE(offset_count, 20u);  // player's share
  EXPECT_LT(offset_count, center_count);
  EXPECT_EQ(results.size(), 100);
}

TEST_F(StreamerTest, BasicPerformanceTest) {
  const size_t kIterations = 1000;
  const size_t kEntities = 10000;
  const size_t kPlayers = 50;

  // Performs a basic performance test for the streamer under conditions that the server might
  // run in: 10k randomly distributed entities for 50 players, on an optimised Streamer. The
  // actual streaming routine is ran a thousand times to get a more detailed result.
  Streamer streamer(/* max_visible= */ 1000, /* max_distance= */ 300);
  for (uint32_t entity_id = 1; entity_id <= kEntities; ++entity_id)
    streamer.Add(entity_id, RandomX(), RandomY(), RandomZ());
  
  std::vector<std::vector<StreamerUpdate>> cases;
  for (uint32_t case_index = 0; case_index < kIterations; ++case_index) {
    std::vector<StreamerUpdate> updates;

    for (uint32_t playerid = 0; playerid <= kPlayers; ++playerid) {
      StreamerUpdate update;
      update.playerid = playerid;
      update.position[0] = RandomX();
      update.position[1] = RandomY();
      update.position[2] = RandomZ();

      updates.push_back(std::move(update));
    }

    cases.push_back(std::move(updates));
  }

  uint32_t total_entities = 0;

  // Now run all the |cases|. The number of found entities will be added to a local variable to
  // ensure that there is *some* output, as well as to avoid overly aggressive optimizations.
  double query_start = base::monotonicallyIncreasingTime();
  {
    for (const auto& updates : cases)
      total_entities += streamer.Stream(updates).size();
  }
  double query_end = base::monotonicallyIncreasingTime();

  ASSERT_GT(total_entities, cases.size() * 500);  // require a minimum average of 50% coverage
  LOG(INFO) << "Queried " << kEntities << " entities for " << kPlayers << " players 1000x in "
            << (query_end - query_start) << "ms, yielding " << total_entities << " results.";
}

}  // namespace streamer
}  // namespace bindings
