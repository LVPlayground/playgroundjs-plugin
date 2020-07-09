// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_ARGUMENTS_H_
#define PLAYGROUND_PLUGIN_ARGUMENTS_H_

#include <any>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"

namespace plugin {

struct Callback;

// The Arguments class represents the arguments passed to a Pawn callback, which are to be forwarded
// to the v8 runtime (or potentially other consumers).
class Arguments {
  using ArrayType = std::vector<uint32_t>;

 public:
  Arguments();
  Arguments(const Arguments&) = delete;
  Arguments(Arguments&&);
  ~Arguments();

  void operator=(const Arguments&) = delete;
  void operator=(Arguments&&) noexcept;

  Arguments Copy() const;

  void AddInteger(const std::string& name, int value);
  void AddFloat(const std::string& name, float value);
  void AddString(const std::string& name, const std::string& value);
  void AddArray(const std::string& name, const ArrayType& value);

  int GetInteger(const std::string& name) const;
  float GetFloat(const std::string& name) const;
  const std::string& GetString(const std::string& name) const;
  const ArrayType& GetArray(const std::string& name) const;

  size_t size() const { return values_.size(); }
  void clear() { values_.clear(); }

 private:
  int64_t instance_id_;
  std::unordered_map<std::string, std::any> values_;
};

// Returns a textual callback representation visualizing the call that is being made in Pawn. This
// is mostly convenient for debugging purposes.
std::string GetCallbackRepresentation(const Callback& callback, const Arguments& arguments);

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_ARGUMENTS_H_
