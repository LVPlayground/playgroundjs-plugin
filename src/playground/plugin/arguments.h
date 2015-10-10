// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_ARGUMENTS_H_
#define PLAYGROUND_PLUGIN_ARGUMENTS_H_

#include <string>
#include <unordered_map>

#include "base/macros.h"

namespace plugin {

struct Callback;

// The Arguments class represents the arguments passed to a Pawn callback, which are to be forwarded
// to the v8 runtime (or potentially other consumers).
//
// TODO: This class can be implemented *significantly* more optimized, but that's not where I want
// to spend my time right now. Feel free to update this if you so desire :)
class Arguments {
 public:
  Arguments();
  ~Arguments();

  void AddInteger(const std::string& name, int value);
  void AddFloat(const std::string& name, float value);
  void AddString(const std::string& name, const std::string& value);

  int GetInteger(const std::string& name) const;
  float GetFloat(const std::string& name) const;
  const std::string& GetString(const std::string& name) const;

  size_t size() const {
    return integer_values_.size() +
           float_values_.size() +
           string_values_.size();
  }

  void clear() {
    integer_values_.clear();
    float_values_.clear();
    string_values_.clear();
  }

 private:
  std::unordered_map<std::string, int> integer_values_;
  std::unordered_map<std::string, float> float_values_;
  std::unordered_map<std::string, std::string> string_values_;

  DISALLOW_COPY_AND_ASSIGN(Arguments);
};

// Returns a textual callback representation visualizing the call that is being made in Pawn. This
// is mostly convenient for debugging purposes.
std::string GetCallbackRepresentation(const Callback& callback, const Arguments& arguments);

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_ARGUMENTS_H_
