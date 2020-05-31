// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/encoding.h"
#include "base/logging.h"

#include "gtest/gtest.h"

TEST(EncodingTest, RoundTrip) {
  const char kExampleText[] =
    "\x46\x6F\x6F\x20\xC2\xA9\x20\x62\x61\x72\x20\xF0\x9D\x8C\x86\x20\x62\x61\x7A\x20\xE2\x98\x83\x20\x71\x75\x78\x00";

  const std::string input = std::string(kExampleText);
  const std::string utf8 = fromAnsi(input);
  const std::string result = toAnsi(input);

  LOG(INFO) << result;
}
