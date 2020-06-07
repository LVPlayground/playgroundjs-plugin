// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/modules/socket/socket_open_options.h"

#include "base/logging.h"
#include "bindings/utilities.h"

namespace bindings {
namespace socket {

namespace {

bool ReadNumberFromObject(v8::Local<v8::Context> context,
                          v8::Local<v8::Object> object,
                          const char* field,
                          int32_t* number) {
  CHECK(number);

  v8::Local<v8::String> v8Field = v8String(field);
  if (!object->HasOwnProperty(context, v8Field).ToChecked())
    return false;

  v8::MaybeLocal<v8::Value> maybe = object->Get(context, v8Field);
  v8::Local<v8::Value> value;

  if (maybe.IsEmpty() || !maybe.ToLocal(&value))
    return false;

  v8::Maybe<int32_t> maybe_number = value->Int32Value(context);
  return maybe_number.To(number);
}

bool ReadStringFromObject(v8::Local<v8::Context> context,
                          v8::Local<v8::Object> object,
                          const char* field,
                          std::string* string) {
  CHECK(string);

  v8::Local<v8::String> v8Field = v8String(field);
  if (!object->HasOwnProperty(context, v8Field).ToChecked())
    return false;

  v8::MaybeLocal<v8::Value> maybe = object->Get(context, v8Field);
  v8::Local<v8::Value> value;

  if (maybe.IsEmpty() || !maybe.ToLocal(&value) || !value->IsString())
    return false;

  *string = toString(value);
  return true;
}

}  // namespace

bool ParseSocketOpenOptions(v8::Local<v8::Value> value, SocketOpenOptions* options) {
  CHECK(options);

  auto context = GetContext();

  if (!value->IsObject()) {
    ThrowException("unable to call open(): argument 1 is expected to be an object.");
    return false;
  }

  v8::Local<v8::Object> dict = value.As<v8::Object>();

  if (!ReadStringFromObject(context, dict, "host", &options->host) &&
      !ReadStringFromObject(context, dict, "ip", &options->host)) {
    ThrowException("unable to call open(): missing or invalid `host` option.");
    return false;
  }

  if (!ReadStringFromObject(context, dict, "path", &options->path))
    options->path = "/";

  if (!ReadNumberFromObject(context, dict, "port", &options->port)) {
    ThrowException("unable to call open(): missing or invalid `ip` option.");
    return false;
  }

  if (!ReadNumberFromObject(context, dict, "timeout", &options->timeout))
    options->timeout = 30;

  options->ssl = SocketSSLMode::kNone;
  {
    std::string ssl;
    if (ReadStringFromObject(context, dict, "ssl", &ssl) && !FromString(ssl, &options->ssl)) {
      ThrowException("unable to call open(): invalid `ssl` option.");
      return false;
    }
  }

  return true;
}

}  // namespace socket
}  // namespace bindings
