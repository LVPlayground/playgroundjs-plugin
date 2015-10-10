// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/callback_parser.h"

#include "base/string_piece.h"
#include "gtest/gtest.h"

namespace plugin {

TEST(CallbackParserTest, ParseLineNoArguments) {
  Callback callback;

  std::unique_ptr<CallbackParser> parser(new CallbackParser());
  ASSERT_TRUE(parser->ParseLine("forward OnGameModeInit();", &callback));

  EXPECT_EQ("OnGameModeInit", callback.name);
  EXPECT_EQ(0u, callback.arguments.size());
  EXPECT_FALSE(callback.cancelable);
}

TEST(CallbackParserTest, ParseLineCancelableAnnotation) {
  Callback callback;

  std::unique_ptr<CallbackParser> parser(new CallbackParser());
  ASSERT_TRUE(parser->ParseLine("[Cancelable] forward OnGameModeExit();", &callback));

  EXPECT_EQ("OnGameModeExit", callback.name);
  EXPECT_EQ(0u, callback.arguments.size());
  EXPECT_TRUE(callback.cancelable);
}

TEST(CallbackParserTest, ParseLineUnknownAnnotation) {
  Callback callback;

  std::unique_ptr<CallbackParser> parser(new CallbackParser());
  ASSERT_TRUE(parser->ParseLine("[CatsAreAwesome, Cancelable] forward OnFilterScriptInit();", &callback));

  EXPECT_EQ("OnFilterScriptInit", callback.name);
  EXPECT_EQ(0u, callback.arguments.size());
  EXPECT_TRUE(callback.cancelable);
}

TEST(CallbackParserTest, ParseLineOneArgument) {
  Callback callback;

  std::unique_ptr<CallbackParser> parser(new CallbackParser());
  ASSERT_TRUE(parser->ParseLine("forward OnPlayerUpdate(playerid);", &callback));

  EXPECT_EQ("OnPlayerUpdate", callback.name);
  ASSERT_EQ(1u, callback.arguments.size());

  const auto& first_argument = callback.arguments[0];
  EXPECT_EQ("playerid", first_argument.first);
  EXPECT_EQ(ARGUMENT_TYPE_INT, first_argument.second);

  EXPECT_FALSE(callback.cancelable);
}

TEST(CallbackParserTest, ParseLineMultipleArguments) {
  Callback callback;

  std::unique_ptr<CallbackParser> parser(new CallbackParser());
  ASSERT_TRUE(parser->ParseLine("forward OnMyCustomCallback(playerid, Float:health, name[]);", &callback));

  EXPECT_EQ("OnMyCustomCallback", callback.name);
  ASSERT_EQ(3u, callback.arguments.size());

  const auto first_argument = callback.arguments[0];
  EXPECT_EQ("playerid", first_argument.first);
  EXPECT_EQ(ARGUMENT_TYPE_INT, first_argument.second);

  const auto second_argument = callback.arguments[1];
  EXPECT_EQ("health", second_argument.first);
  EXPECT_EQ(ARGUMENT_TYPE_FLOAT, second_argument.second);

  const auto third_argument = callback.arguments[2];
  EXPECT_EQ("name", third_argument.first);
  EXPECT_EQ(ARGUMENT_TYPE_STRING, third_argument.second);

  EXPECT_FALSE(callback.cancelable);
}

TEST(CallbackParserTest, ParseWithWhitespace) {
  Callback callback;

  std::unique_ptr<CallbackParser> parser(new CallbackParser());
  ASSERT_TRUE(parser->ParseLine(" [ Cancelable ]  forward  OnMyCustomCallback  ( playerid , Float : health , name [ ] ) ; ", &callback));

  EXPECT_EQ("OnMyCustomCallback", callback.name);
  EXPECT_EQ(3u, callback.arguments.size());
  EXPECT_TRUE(callback.cancelable);
}

TEST(CallbackParserTest, ParseFromString) {
  const char callback_content[] =
      "forward OnMyCallback();\n"
      "forward OnMySecondCallback();";

  std::unique_ptr<CallbackParser> parser(CallbackParser::FromString(callback_content));
  ASSERT_TRUE(parser != nullptr);

  EXPECT_EQ(2u, parser->size());
}

TEST(CallbackParserTest, ParseFromFile) {
  // TODO: Write a test where a file is actually loaded.
}

TEST(CallbackParserTest, ParseIgnoreComments) {
  const char callback_content[] =
      "forward OnMyCallback();\n"
      "forward OnMySecondCallback();\n"
      "# forward OnMyThirdCallback();\n"
      "  # forward OnMyFourthCallback();\n"
      "// forward OnMyFifthCallback();\n"
      "forward OnMySixthCallback();";

  std::unique_ptr<CallbackParser> parser(CallbackParser::FromString(callback_content));
  ASSERT_TRUE(parser != nullptr);

  EXPECT_EQ(3u, parser->size());
}

TEST(CallbackParserTest, Iterable) {
  const char callback_content[] =
      "forward OnMyCallback();\n"
      "forward OnMySecondCallback();";

  std::unique_ptr<CallbackParser> parser(CallbackParser::FromString(callback_content));
  ASSERT_TRUE(parser != nullptr);

  std::set<std::string> remaining_callbacks({ "OnMyCallback", "OnMySecondCallback" });
  ASSERT_EQ(remaining_callbacks.size(), parser->size());

  for (auto& callback : *parser)
    remaining_callbacks.erase(callback.name);
  
  EXPECT_EQ(0u, remaining_callbacks.size());
}

}  // namespace plugin
