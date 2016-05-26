// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "playground/plugin/native_parser.h"

#include <fstream>
#include <set>
#include <streambuf>

#include "base/file_path.h"
#include "base/logging.h"
#include "base/string_piece.h"
#include "bindings/provided_natives.h"
#include "plugin/native_parameters.h"
#include "plugin/sdk/amx.h"

namespace plugin {
namespace {

// The whitespace characters as specified by CSS 2.1.
const char kWhitespaceCharacters[] = "\x09\x0A\x0C\x0D\x20";

// The global instance of the NativeParser object. Required for the native functions themselves.
NativeParser* g_native_parser = nullptr;

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

// Returns whether |character| is valid for use in a native function name.
bool IsValidCharacter(char character) {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') || character == '_';
}

// Registers |N| native functions, creates |N| functions that will automagically forward the call
// to the ProvidedNatives bindings class when called with minimal overhead.
template <size_t N> struct NativeRegistrar {
  static void Register() {
    DCHECK(g_native_parser);

    constexpr size_t native_index = NativeParser::kMaxNatives - N;
    if (native_index < g_native_parser->size()) {
      AMX_NATIVE_INFO* native = &g_native_parser->GetNativeTable()[native_index];
      native->name = _strdup(g_native_parser->at(native_index).c_str());
      native->func = &NativeRegistrar<N>::Invoke;
    }

    NativeRegistrar<N - 1>::Register();
  }

  static int32_t AMX_NATIVE_CALL Invoke(AMX* amx, cell* params) {
    DCHECK(g_native_parser);

    constexpr size_t native_index = NativeParser::kMaxNatives - N;
    return bindings::ProvidedNatives::GetInstance()->Call(g_native_parser->at(native_index), NativeParameters(amx, params));
  }
};

template <> struct NativeRegistrar<0> {
  static void Register() {}
};

}  // namespace

std::unique_ptr<NativeParser> NativeParser::FromFile(const base::FilePath& filename) {
  std::ifstream file(filename.value().c_str());
  if (!file.is_open() || file.fail())
    return nullptr;

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  std::unique_ptr<NativeParser> parser(new NativeParser);
  if (!parser->Parse(content))
    return nullptr;

  return parser;
}

NativeParser::NativeParser() {
  g_native_parser = this;
  memset(native_table_, 0, sizeof(native_table_));
}

NativeParser::~NativeParser() {
  g_native_parser = nullptr;
}

size_t NativeParser::size() const {
  return natives_.size();
}

const std::string& NativeParser::at(size_t index) const {
  DCHECK(natives_.size() > index);
  return natives_[index];
}

bool NativeParser::Parse(const std::string& content) {
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
    }
    else {
      line = content_lines.substr(start, end - start);
      start = end + 1;
    }

    line = Trim(line);
    if (line.empty())
      continue;  // do not process empty lines.

    if (line.starts_with("#") || line.starts_with("//"))
      continue;  // comment line.

    if (!ParseLine(line))
      return false;
  }

  if (natives_.size() > kMaxNatives) {
    LOG(ERROR) << "No more than " << kMaxNatives << " may be defined in natives.txt.";
    return false;
  }

  bindings::ProvidedNatives::GetInstance()->SetNatives(natives_);
  BuildNativeTable();

  return true;
}

bool NativeParser::ParseLine(base::StringPiece line) {
  for (size_t i = 0; i < line.length(); ++i) {
    if (!IsValidCharacter(line[i])) {
      LOG(ERROR) << "Invalid native function name: " << line.as_string();
      return false;
    }
  }

  const std::string name = line.as_string();
  if (std::find(natives_.begin(), natives_.end(), name) != natives_.end()) {
    LOG(ERROR) << "Native has been listed multiple times: " << line.as_string();
    return false;
  }

  natives_.push_back(name);
  return true;
}

void NativeParser::BuildNativeTable() {
  NativeRegistrar<kMaxNatives>::Register();
}

}  // namespace plugin
