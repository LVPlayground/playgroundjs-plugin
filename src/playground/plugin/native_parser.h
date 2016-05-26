// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_NATIVE_PARSER_H_
#define PLAYGROUND_PLUGIN_NATIVE_PARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/string_piece.h"
#include "plugin/sdk/amx.h"

namespace base {
class FilePath;
}

namespace plugin {

// Parses the list of native functions supported by the plugin from a given input file. They
// will be registered when the AMX files load.
class NativeParser {
 public:
  // The maximum number of native functions that may be defined by the parser.
  static constexpr size_t kMaxNatives = 255;
   
  // Loads the list of native functions from |filename|.
  static std::unique_ptr<NativeParser> FromFile(const base::FilePath& filename);

  ~NativeParser();

  // Returns the number of natives that have been stored in the parser.
  size_t size() const;

  // Returns the name of the native function at |index|.
  const std::string& at(size_t index) const;

  // Gets the table of native AMX functions to be shared with the SA-MP server.
  AMX_NATIVE_INFO* GetNativeTable() { return native_table_; }

 private:
  NativeParser();

  // Parses the |content| as the list of natives to be stored in this instance.
  bool Parse(const std::string& content);
  bool ParseLine(base::StringPiece line);

  // Builds the native table based on the natives that have been loaded.
  void BuildNativeTable();
  
  // Set of the native functions that have been registered by the parser.
  std::vector<std::string> natives_;

  // The native table has to be of a fixed size and should be frozen after all native functions
  // have been loaded. This will be used by the SA-MP server to load natives from this module.
  AMX_NATIVE_INFO native_table_[kMaxNatives + 1];

  DISALLOW_COPY_AND_ASSIGN(NativeParser);
};

} // namespace plugin

#endif  // PLAYGROUND_PLUGIN_NATIVE_PARSER_H_
