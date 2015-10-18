// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/callback_parser.h"

#include <fstream>
#include <set>
#include <streambuf>

#include "base/file_path.h"
#include "base/string_piece.h"

namespace plugin {

namespace {

// The known annotations as they will be parsed by ParseAnnotations().
const char kAnnotationCancelable[] = "Cancelable";
const char kAnnotationReturnOne[] = "ReturnOne";

// The whitespace characters as specified by CSS 2.1.
const char kWhitespaceCharacters[] = "\x09\x0A\x0C\x0D\x20";

// Removes all whitespace from the front and back of |string|, and returns the result.
base::StringPiece Trim(const base::StringPiece& input) {
  if (input.empty())
    return input;

  const size_t first_good_char = input.find_first_not_of(kWhitespaceCharacters);
  const size_t last_good_char = input.find_last_not_of(kWhitespaceCharacters);

  if (first_good_char == base::StringPiece::npos ||
      last_good_char == base::StringPiece::npos)
    return base::StringPiece();

  return input.substr(first_good_char, last_good_char - first_good_char + 1);
}

// Parses any leading annotations in |line| and applies the known ones to |callback|. Returns
// true unless there are annotations, and parsing of them fails.
bool ParseAnnotations(base::StringPiece* line, Callback* callback) {
  if (!line->starts_with("["))
    return true;  // there are no annotations.

  std::set<base::StringPiece> annotations;
  
  size_t index = 0, last_offset = 1;
  while (++index < line->length()) {
    char character = (*line)[index];
    if (character == ',' || character == ']') {
      annotations.insert(Trim(line->substr(last_offset, index - last_offset)));
      if (character == ']')
        break;

      last_offset = index + 1;
      continue;
    }
  }

  // Iterate over the found annotations to identify the known ones, and mark them as such
  // in the |callback| passed on to this function.
  for (const auto& annotation : annotations) {
    if (annotation == kAnnotationCancelable)
      callback->cancelable = true;
    else if (annotation == kAnnotationReturnOne)
      callback->return_value = 1;

    // TODO: Parse additional annotations here.
  }

  if ((*line)[index] != ']')
    return false;

  *line = Trim(line->substr(index + 1));
  return true;
}

// Parses |argument| per the syntax of a single argument to a callback, storing the result
// in the arguments map part of |callback|.
bool ParseArgument(const base::StringPiece& input_argument, Callback* callback) {
  CallbackArgumentType type = ARGUMENT_TYPE_INT;

  base::StringPiece argument(Trim(input_argument));

  // First see if the argument is prepended by a type name. We only special-case Float, as
  // other values are simply named aliases of Pawn 32-bit signed integers.
  size_t type_offset = argument.find_first_of(':');
  if (type_offset != base::StringPiece::npos) {
    if (argument.substr(0, type_offset) == "Float")
      type = ARGUMENT_TYPE_FLOAT;

    argument = Trim(argument.substr(type_offset + 1));
  }

  // Then see if the argument is a string. Right now we only support strings of undefined
  // size, which are identified by the consecutive characters "[" and "]".
  size_t string_offset = argument.find_first_of('[');
  if (string_offset != base::StringPiece::npos) {
    size_t string_end = argument.find_first_of(']', string_offset);
    if (string_end == base::StringPiece::npos)
      return false;  // the closing bracket does not exist.

    if (type != ARGUMENT_TYPE_INT)
      return false;  // no such thing as float strings.

    argument = Trim(argument.substr(0, string_offset));
    type = ARGUMENT_TYPE_STRING;
  }

  callback->arguments.push_back(std::make_pair(argument.as_string(), type));
  return true;
}

}  // namespace

// static
CallbackParser* CallbackParser::FromFile(const base::FilePath& filename) {
  std::ifstream file(filename.value().c_str());
  if (!file.is_open() || file.fail())
    return nullptr;

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  std::unique_ptr<CallbackParser> parser(new CallbackParser);
  if (!parser->Parse(content))
    return nullptr;

  return parser.release();
}

// static
CallbackParser* CallbackParser::FromString(const std::string& content) {
  std::unique_ptr<CallbackParser> parser(new CallbackParser);
  if (!parser->Parse(content))
    return nullptr;

  return parser.release();
}

CallbackParser::CallbackParser() {}

CallbackParser::~CallbackParser() {}

bool CallbackParser::Parse(const std::string& content) {
  base::StringPiece content_lines(content);
  if (!content_lines.length())
    return true;  // empty contents

  size_t start = 0;
  while (start != base::StringPiece::npos) {
    size_t end = content_lines.find_first_of("\n", start);

    base::StringPiece line;
    if (end == base::StringPiece::npos) {
      line = content_lines.substr(start);
      start = end;
    } else {
      line = content_lines.substr(start, end - start);
      start = end + 1;
    }

    line = Trim(line);
    if (line.empty())
      continue;  // do not process empty lines.

    if (line.starts_with("#") || line.starts_with("//"))
      continue;  // comment line.

    Callback callback;
    if (!ParseLine(line, &callback))
      return false;

    callbacks_.push_back(callback);
  }

  return true;
}

bool CallbackParser::ParseLine(const base::StringPiece& line, Callback* callback) const {
  Callback candidate_callback;

  base::StringPiece parsing_line(Trim(line));
  if (!ParseAnnotations(&parsing_line, &candidate_callback)) {
    LOG(WARNING) << "Syntax error: Unable to parse annotations. (\"" << line << "\").";
    return false;
  }

  if (!parsing_line.starts_with("forward ")) {
    LOG(WARNING) << "Syntax error: Expected keyword \"forward\". (\"" << line << "\").";
    return false;
  }

  parsing_line = Trim(parsing_line.substr(8 /* strlen(forward ) */));

  size_t arguments_offset = parsing_line.find_first_of('(');
  if (arguments_offset == base::StringPiece::npos) {
    LOG(WARNING) << "Syntax error: Unable to find the arguments. (\"" << line << "\").";
    return false;
  }

  // Store the callback's name, remove the name and suffix from the |parsing_line|.
  Trim(parsing_line.substr(0, arguments_offset)).CopyToString(&candidate_callback.name);
  parsing_line.remove_prefix(arguments_offset + 1);

  if (parsing_line.ends_with(";"))
    parsing_line = Trim(parsing_line.substr(0, parsing_line.length() - 1));

  if (!parsing_line.ends_with(")")) {
    LOG(WARNING) << "Syntax error: Unable to find end of the arguments. (\"" << line << "\").";
    return false;
  }

  parsing_line = Trim(parsing_line.substr(0, parsing_line.length() - 1));
  
  // Parse the arguments from |parsing_line|. There could be none, or many.
  while (parsing_line.length()) {
    size_t separator_offset = parsing_line.find_first_of(',');
    if (!ParseArgument(parsing_line.substr(0, separator_offset), &candidate_callback))
      return false;
    
    if (separator_offset == base::StringPiece::npos)
      break;

    parsing_line = Trim(parsing_line.substr(separator_offset + 1));
  }

  // Parsing of the callback was successful. Move the results to |callback|.
  callback->arguments.swap(candidate_callback.arguments);
  callback->name.swap(candidate_callback.name);
  callback->return_value = candidate_callback.return_value;
  callback->cancelable = candidate_callback.cancelable;

  return true;
}

const Callback* CallbackParser::Find(const std::string& name) const {
  for (const auto& callback : callbacks_) {
    if (callback.name == name)
      return &callback;
  }

  return nullptr;
}

}  // namespace plugin
