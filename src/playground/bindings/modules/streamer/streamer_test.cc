// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/streamer/streamer.h"

#include "base/logging.h"
#include "base/time.h"
#include "gtest/gtest.h"

#include <memory>
#include <random>

namespace streamer {

class StreamerTest : public ::testing::Test {
 public:
  StreamerTest()
      : distribution_(-3000, 3000),
        height_distribution_(-10, 300) {}

 protected:
  inline double RandomX() { return distribution_(random_generator_); }
  inline double RandomY() { return distribution_(random_generator_); }
  inline double RandomZ() { return height_distribution_(random_generator_); }

   // Initializes the streamer based on the given settings.
  void CreateStreamer(uint32_t max_visible, double stream_distance) {
    streamer_.reset(new Streamer(max_visible, stream_distance));
  }

  // Creates |count| random objects within the GTA coordinate space.
  void CreateRandomObjects(size_t count) {
    DCHECK(streamer_);
    for (size_t i = 0; i < count; ++i)
      streamer_->Add(i, RandomX(), RandomY(), RandomZ());
  }

  std::default_random_engine random_generator_;

  std::uniform_real_distribution<double> distribution_;
  std::uniform_real_distribution<double> height_distribution_;

  std::unique_ptr<Streamer> streamer_;
};

TEST_F(StreamerTest, HonoursMaxVisible) {
  // This test verifies that the Streamer object honours the |max_visible| parameter given to
  // the constructor, which limits the maximum number of objects to return from a query.
  constexpr size_t TEST_OBJECT_COUNT = 10000;
  constexpr double TEST_STREAM_DISTANCE = 10000;
  constexpr size_t TEST_KNN_VISIBLE = 500;

  CreateStreamer(TEST_KNN_VISIBLE, TEST_STREAM_DISTANCE);
  CreateRandomObjects(TEST_OBJECT_COUNT);

  auto results = streamer_->Stream(RandomX(), RandomY(), RandomZ());
  EXPECT_LE(results.size(), TEST_KNN_VISIBLE);
}

TEST_F(StreamerTest, HonoursStreamDistance) {
  // This test verifies that the Streamer object honours the |stream_distance| parameter given
  // to the constructor, which limits the maximum distance of objects.
  constexpr double TEST_STREAM_DISTANCE = 50.5;
  constexpr size_t TEST_KNN_VISIBLE = 500;

  CreateStreamer(TEST_KNN_VISIBLE, TEST_STREAM_DISTANCE);
  
  streamer_->Add(0, 100, 100, 0);
  streamer_->Add(1, 150, 100, 0);
  streamer_->Add(2, 200, 100, 0);

  streamer_->Add(3, 100, 150, 0);
  streamer_->Add(4, 150, 150, 0);
  streamer_->Add(5, 200, 150, 0);

  streamer_->Add(6, 100, 200, 0);
  streamer_->Add(7, 150, 200, 0);
  streamer_->Add(8, 200, 200, 0);

  auto results = streamer_->Stream(150, 150, 0);
  ASSERT_EQ(results.size(), 5);
}

TEST_F(StreamerTest, HundredThousandObjectPerformance) {
  // This test is a performance test that creates a hundred thousand objects for the streamer,
  // then does two thousand kNN queries for the five hundred nearest objects.
  constexpr size_t TEST_OBJECT_COUNT = 100000;
  constexpr double TEST_STREAM_DISTANCE = 1000;
  constexpr size_t TEST_KNN_QUERIES = 20000;
  constexpr size_t TEST_KNN_VISIBLE = 500;

  CreateStreamer(TEST_KNN_VISIBLE, TEST_STREAM_DISTANCE);

  double insertion_start = base::monotonicallyIncreasingTime();
  CreateRandomObjects(TEST_OBJECT_COUNT);
  double insertion_end = base::monotonicallyIncreasingTime();

  LOG(INFO) << "Inserting " << TEST_OBJECT_COUNT << " objects: "
            << (insertion_end - insertion_start) << "ms.";

  std::uniform_real_distribution<double> distribution(-3000, 3000);

  size_t query_results = 0;
  double query_start = base::monotonicallyIncreasingTime();

  for (size_t i = 0; i < TEST_KNN_QUERIES; ++i) {
    query_results += streamer_->Stream(RandomX(), RandomY(), RandomZ()).size();
  }

  double query_end = base::monotonicallyIncreasingTime();

  LOG(INFO) << "Querying the nearest " << TEST_KNN_VISIBLE << " objects "
            << TEST_KNN_QUERIES << " times: " << (query_end - query_start) << "ms.";

  EXPECT_EQ(query_results, query_results);  // avoid compiler optimizations
}

}  // namespace streamer
