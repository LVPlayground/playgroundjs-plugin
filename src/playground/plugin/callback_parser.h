// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_CALLBACK_PARSER_H_
#define PLAYGROUND_PLUGIN_CALLBACK_PARSER_H_

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "gtest/gtest_prod.h"
#include "plugin/callback.h"

namespace base {
class FilePath;
class StringPiece;
}

namespace plugin {

// Supported SA-MP callbacks are stored in an on-disk file in order to prevent needing to update
// the plugin every time a new callback is added. This also allows the v8 runtime to receive
// callbacks triggered by other plugins loaded in the SA-MP server.
//
// The file format is simple: each line contains the definition of a single callback, in Pawn
// syntax, optionally decorated with annotations. These annotations enable custom behaviour, such
// as marking the event as cancelable, in which case the gamemode will never be informed of it.
//
// For example, take the following callback definition:
//
//     [Cancelable] forward OnPlayerText(playerid, text[]);
//
// This will result in an "onplayertext" event on the global scope in the v8 runtime, which
// events may be bound to in a DOM-esque way. This event will be invoked with an |event| interface
// that has the following signature (in WebIDL):
//
// interface PlayerTextEvent {
//     readonly attribute long playerid;
//     readonly attribute USVString text;
//
//     readonly attribute defaultPrevented;
//     void preventDefault();
// }
//
// Calling the preventDefault() method will prevent the gamemode from receiving the event.
// Other annotations may become available in the future, and will be documented here accordingly.
class CallbackParser {
 public:
  // Creates a CallbackParser instance for the contents from |filename|. A nullptr will be
  // returned if the file cannot be opened for reading, or cannot be parsed correctly.
  static CallbackParser* FromFile(const base::FilePath& filename);

  // Creates a CallbackParser for |content|. A nullptr will be returned if the |content|
  // cannot be parsed correctly.
  static CallbackParser* FromString(const std::string& content);

  ~CallbackParser();

  // TODO: This method is O(n), but could be O(1) by sacrificing some memory.
  const Callback* Find(const std::string& name) const;

  size_t size() const { return callbacks_.size(); }

  std::vector<Callback>::const_iterator begin() const { return callbacks_.begin(); }
  std::vector<Callback>::const_iterator end() const { return callbacks_.end(); }

  const std::vector<Callback>& callbacks() const { return callbacks_; }

 private:
  FRIEND_TEST(CallbackParserTest, ParseLineNoArguments);
  FRIEND_TEST(CallbackParserTest, ParseLineCancelableAnnotation);
  FRIEND_TEST(CallbackParserTest, ParseLineUnknownAnnotation);
  FRIEND_TEST(CallbackParserTest, ParseLineOneArgument);
  FRIEND_TEST(CallbackParserTest, ParseLineMultipleArguments);
  FRIEND_TEST(CallbackParserTest, ParseWithWhitespace);

  CallbackParser();

  // Parses |content| using this parser. Returns whether |content| could be parsed successfully.
  bool Parse(const std::string& content);

  // Parses any forward function declarations from |line|. The expected syntax is as follows:
  //
  //     [callback]     = [annotations]? "forward" \w+ "(" [argument ","?]* ")" ";"
  //     [annotations]  = "[" [ \w+ ","? ]+ "]"
  //     [argument]     = (\w+ ":")? \w+ (\[\])?
  //
  // Any other syntax will be considered a syntax error.
  bool ParseLine(const base::StringPiece& line, Callback* callback) const;

  // Vector containing the callbacks which were parsed using this parser.
  std::vector<Callback> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(CallbackParser);
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_CALLBACK_PARSER_H_
