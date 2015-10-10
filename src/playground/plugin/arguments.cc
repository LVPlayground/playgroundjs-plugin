// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/arguments.h"

#include <sstream>

#include "plugin/callback.h"

namespace plugin {

namespace {

const std::string g_empty_string;

}

Arguments::Arguments() {}

Arguments::~Arguments() {}

void Arguments::AddInteger(const std::string& name, int value) {
  integer_values_[name] = value;
}

void Arguments::AddFloat(const std::string& name, float value) {
  float_values_[name] = value;
}

void Arguments::AddString(const std::string& name, const std::string& value) {
  string_values_[name] = value;
}

int Arguments::GetInteger(const std::string& name) const {
  auto value_iter = integer_values_.find(name);
  if (value_iter == integer_values_.end())
    return -1;

  return value_iter->second;
}

float Arguments::GetFloat(const std::string& name) const {
  auto value_iter = float_values_.find(name);
  if (value_iter == float_values_.end())
    return -1.0;

  return value_iter->second;
}

const std::string& Arguments::GetString(const std::string& name) const {
  auto value_iter = string_values_.find(name);
  if (value_iter == string_values_.end())
    return g_empty_string;

  return value_iter->second;
}

std::string GetCallbackRepresentation(const Callback& callback, const Arguments& arguments) {
  std::stringstream representation;

  representation << callback.name;
  representation << "(";

  size_t index = 0;
  for (const auto& argument : callback.arguments) {
    switch (argument.second) {
    case ARGUMENT_TYPE_INT:
      representation << arguments.GetInteger(argument.first);
      break;
    case ARGUMENT_TYPE_FLOAT:
      representation << arguments.GetFloat(argument.first);
      break;
    case ARGUMENT_TYPE_STRING:
      representation << "\"" << arguments.GetString(argument.first) << "\"";
      break;
    }

    if (++index < callback.arguments.size())
      representation << ", ";
  }

  representation << ")";
  
  return representation.str();
}

}  // namespace plugin
