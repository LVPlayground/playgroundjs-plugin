// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/arguments.h"

#include <sstream>

#include "base/memory.h"
#include "plugin/callback.h"

namespace plugin {

namespace {

const std::string g_empty_string;

int64_t g_argumentsLive = 0;
int64_t g_argumentsInstanceId = 0;

}  // namespace

Arguments::Arguments()
    : instance_id_(++g_argumentsInstanceId) {
  LOG(ALLOC) << "Arguments " << instance_id_ << " (live: " << (++g_argumentsLive) << ")";
}

Arguments::Arguments(Arguments&& other) {
  instance_id_ = other.instance_id_;
  values_ = std::move(other.values_);

  other.instance_id_ = -1;
}

Arguments::~Arguments() {
  if (instance_id_ != -1)
    LOG(ALLOC) << "~Arguments " << instance_id_ << " (live: " << (--g_argumentsLive) << ")";;
}

void Arguments::operator=(Arguments&& other) noexcept {
  instance_id_ = other.instance_id_;
  values_ = std::move(other.values_);

  other.instance_id_ = -1;
}

Arguments Arguments::Copy() const {
  Arguments copy;
  copy.values_ = values_;

  return copy;
}

void Arguments::AddInteger(const std::string& name, int value) {
  values_[name] = std::make_any<int32_t>(value);
}

void Arguments::AddFloat(const std::string& name, float value) {
  values_[name] = std::make_any<float>(value);
}

void Arguments::AddString(const std::string& name, const std::string& value) {
  values_[name] = std::make_any<std::string>(value);
}

int Arguments::GetInteger(const std::string& name) const {
  auto value_iter = values_.find(name);
  if (value_iter == values_.end() || value_iter->second.type() != typeid(int32_t))
    return -1;

  return std::any_cast<int32_t>(value_iter->second);
}

float Arguments::GetFloat(const std::string& name) const {
  auto value_iter = values_.find(name);
  if (value_iter == values_.end() || value_iter->second.type() != typeid(float))
    return -1.0;

  return std::any_cast<float>(value_iter->second);
}

const std::string& Arguments::GetString(const std::string& name) const {
  auto value_iter = values_.find(name);
  if (value_iter == values_.end() || value_iter->second.type() != typeid(std::string))
    return g_empty_string;

  return std::any_cast<const std::string&>(value_iter->second);
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
