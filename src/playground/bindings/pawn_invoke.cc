// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "bindings/pawn_invoke.h"

#include <string.h>

#include "base/logging.h"
#include "bindings/provided_natives.h"
#include "bindings/runtime.h"
#include "bindings/utilities.h"
#include "performance/scoped_trace.h"
#include "plugin/plugin_controller.h"

namespace bindings {

namespace {

// Types of arguments that can be parsed from the signature passed to the pawnInvoke() function,
// see the class-level comment above the PawnInvoke class for more information.
enum SignatureType {
  SIGNATURE_TYPE_ARRAY,
  SIGNATURE_TYPE_FLOAT,
  SIGNATURE_TYPE_FLOAT_REFERENCE,
  SIGNATURE_TYPE_INT,
  SIGNATURE_TYPE_INT_REFERENCE,
  SIGNATURE_TYPE_STRING,
  SIGNATURE_TYPE_STRING_REFERENCE
};

}  // namespace

// Buffer in which we store the data associated with each invocation. This significantly reduces
// the need to do many allocations for each function invocation, at the expense of a little bit
// of memory usage.
struct PawnInvoke::StaticBuffer {
  SignatureType signature[PawnInvoke::kMaxArgumentCount];

  char arguments_format[PawnInvoke::kMaxArgumentCount];
  void* arguments[PawnInvoke::kMaxArgumentCount];
  
  // Integer and floating point values will both be stored in the same buffer, but we must be
  // sure that the compiler gives them the same size.
  static_assert(sizeof(float) == sizeof(int), "Expected sizeof(float) == sizeof(int).");
  int number_values[PawnInvoke::kMaxArgumentCount];

  // Maximum length of a string stored in the static buffer. Total memory required for this is
  // kMaxArgumentCount (24) * kMaxStringLength (2048) = 48 KiB.
  static const size_t kMaxStringLength = 2048;

  char string_values[PawnInvoke::kMaxArgumentCount][kMaxStringLength];

  // Maximum number of entries in an array. The total memory required for this buffer is
  // kMaxArgumentCount (24) * kMaxArrayLength (144) * sizeof(int) = 13.5 KiB.
  static const size_t kMaxArrayLength = 144;

  int array_values[PawnInvoke::kMaxArgumentCount][kMaxArrayLength];
};

PawnInvoke::PawnInvoke(plugin::PluginController* plugin_controller)
    : static_buffer_(static_cast<StaticBuffer*>(calloc(1, sizeof(StaticBuffer)))),
      plugin_controller_(plugin_controller) {}

PawnInvoke::~PawnInvoke() {}

v8::Local<v8::Value> PawnInvoke::Call(const v8::FunctionCallbackInfo<v8::Value>& arguments) {
  DCHECK(arguments.Length() >= 1);
  DCHECK(arguments[0]->IsString());

  v8::Isolate* isolate = arguments.GetIsolate();

  // The first argument contains the name of the function that should be invoked.
  const std::string function = toString(arguments[0]);

  if (!function.length()) {
    ThrowException("unable to execute pawnInvoke(): the function name must not be empty.");
    return v8::Local<v8::Value>();
  }

  // Issue a warning when pawnInvoke() is used for a native that is not provided by JavaScript
  // before the tests have finished running, because tests should not rely on the Pawn code.
  if (!Runtime::FromIsolate(isolate)->IsReady() && !ProvidedNatives::GetInstance()->IsProvided(function))
    LOG(WARNING) << "Called Pawn function " << function << " whilst running the JavaScript tests.";

  // Fast-path for functions that don't take any arguments at all. Immediately invoke the native on
  // the SA-MP server and return whatever it returned to us.
  if (arguments.Length() == 1)
    return v8::Number::New(isolate, plugin_controller_->CallFunction(function));

  if (!arguments[1]->IsString()) {
    ThrowException("unable to execute pawnInvoke(): expected a string for argument 2.");
    return v8::Local<v8::Value>();
  }

  size_t argument_count = 0;
  size_t return_count = 0;

  // Parse the method's signature. The parsed information will be available in |static_buffer_|,
  // whereas argument and return counters will be written to local variables.
  if (!ParseSignature(arguments[1], &argument_count, &return_count)) {
    ThrowException("unable to execute pawnInvoke(): cannot parse the method's signature.");
    return v8::Local<v8::Value>();
  }

  size_t signature_length = argument_count + return_count;

  // Make sure that enough arguments have been passed to satisfy the signature.
  if (arguments.Length() != argument_count + 2) {
    ThrowException("unable to execute pawnInvoke(): " +
                   std::to_string(argument_count + 2) + " arguments required, but only " +
                   std::to_string(arguments.Length()) + " provided.");

    return v8::Local<v8::Value>();
  }

  size_t argument_offset = 0;

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  // Iterate over each of the arguments to verify their types and to set pointers accordingly.
  for (size_t signature_index = 0; signature_index < signature_length; ++signature_index) {
    size_t argument = signature_index + argument_offset;
    size_t index = argument + 2;

    bool type_mismatch = false;
    switch (static_buffer_->signature[argument]) {
    case SIGNATURE_TYPE_ARRAY:
      if (type_mismatch = !arguments[index]->IsArray())
        break;

      {
        int* array_data = static_buffer_->array_values[argument];
        size_t array_length = 0;

        v8::Local<v8::Array> js_array = v8::Local<v8::Array>::Cast(arguments[index]);
        uint32_t js_length = js_array->Length();

        for (uint32_t index = 0; index < js_length; ++index) {
          v8::MaybeLocal<v8::Value> maybe_entry = js_array->Get(context, index);
          if (maybe_entry.IsEmpty())
            continue;

          v8::Local<v8::Value> entry = maybe_entry.ToLocalChecked();
          if (type_mismatch = !entry->IsNumber())
            break;

          if (array_length >= PawnInvoke::StaticBuffer::kMaxArrayLength) {
            ThrowException("unable to execute pawnInvoke(): too many array values for argument " +
                           std::to_string(index));
          }

          array_data[array_length++] = entry->Int32Value(context).ToChecked();
        }

        static_buffer_->arguments[argument] = array_data;
        static_buffer_->arguments_format[argument] = 'a';
      }

      break;

    case SIGNATURE_TYPE_FLOAT:
      if (type_mismatch = !arguments[index]->IsNumber())
        break;

      {
        float float_value = static_cast<float>(arguments[index]->NumberValue(context).ToChecked());
        static_buffer_->number_values[argument] = *reinterpret_cast<int*>(&float_value);
      }

      static_buffer_->arguments[argument] = &static_buffer_->number_values[argument];
      static_buffer_->arguments_format[argument] = 'f';
      break;

    case SIGNATURE_TYPE_FLOAT_REFERENCE:
      static_buffer_->arguments[argument] = &static_buffer_->number_values[argument];
      static_buffer_->arguments_format[argument] = 'r';
      break;

    case SIGNATURE_TYPE_INT:
      if (type_mismatch = !arguments[index]->IsNumber())
        break;

      static_buffer_->number_values[argument] = arguments[index]->Int32Value(context).ToChecked();
      static_buffer_->arguments[argument] = &static_buffer_->number_values[argument];
      static_buffer_->arguments_format[argument] = 'i';
      break;

    case SIGNATURE_TYPE_INT_REFERENCE:
      static_buffer_->arguments[argument] = &static_buffer_->number_values[argument];
      static_buffer_->arguments_format[argument] = 'r';
      break;

    case SIGNATURE_TYPE_STRING:
      {
        v8::String::Utf8Value string(isolate, arguments[index]);

        // If the string is longer than our buffer supports, bail out.
        if (string.length() >= StaticBuffer::kMaxStringLength) {
          ThrowException("unable to execute pawnInvoke(): string overflow for argument " +
                         std::to_string(index));

          return v8::Local<v8::Value>();
        }

        if (string.length())
          memcpy(static_buffer_->string_values[argument], *string, string.length());
        static_buffer_->string_values[argument][string.length()] = 0;
      }

      static_buffer_->arguments[argument] = &static_buffer_->string_values[argument];
      static_buffer_->arguments_format[argument] = 's';
      break;

    case SIGNATURE_TYPE_STRING_REFERENCE:
      static_buffer_->arguments[argument] = &static_buffer_->string_values[argument];
      static_buffer_->arguments_format[argument] = 'a';

      ++argument_offset;
      ++argument;

      static_buffer_->number_values[argument] = StaticBuffer::kMaxStringLength;
      static_buffer_->arguments[argument] = &static_buffer_->number_values[argument];
      static_buffer_->arguments_format[argument] = 'i';
      break;
    }

    // If a type mismatch occurred, an exception will be thrown to JavaScript. At some point we may
    // want to relax the requirements here by using the ES ToType() conversions.
    if (type_mismatch) {
      ThrowException("unable to execute pawnInvoke(): type mismatch for argument " +
                     std::to_string(index) + ".");

      return v8::Local<v8::Value>();
    }
  }

  // The Pawn argument count may not match the JavaScript one, since we substitute the allowed
  // length for string reference arguments automatically.
  const size_t pawn_argument_count = signature_length + argument_offset;

  // Make sure that the argument format string is zero-terminated.
  static_buffer_->arguments_format[pawn_argument_count] = 0;
  DCHECK(strlen(static_buffer_->arguments_format) == pawn_argument_count);

  // Invoke the native SA-MP function. We simply pass the assembled argument format and the
  // array of void* pointers to the intended arguments to the function itself.
  int result = plugin_controller_->CallFunction(function,
                                                static_buffer_->arguments_format,
                                                static_buffer_->arguments);

  // If there are no explicit return values, simply return the |result|.
  if (!return_count || result == -1 /** internal error code **/)
    return v8::Number::New(isolate, static_cast<double>(result));

  // We want to eagerly return a value immediately if there is only one return value. In all other
  // cases, the return values will be stored in an array, and the array will be returned.
  const bool eager_return = (return_count == 1);

  v8::Local<v8::Array> return_array;
  if (!eager_return)
    return_array = v8::Array::New(isolate, return_count);

  size_t stored_return_values = 0;

  // Iterate over the arguments again to find those which were passed as a reference. Those will
  // be considered return values of the JavaScript function.
  for (size_t argument = 0; argument < pawn_argument_count; ++argument) {
    v8::Local<v8::Value> value;

    switch (static_buffer_->signature[argument]) {
    case SIGNATURE_TYPE_FLOAT:
    case SIGNATURE_TYPE_INT:
    case SIGNATURE_TYPE_STRING:
      break;

    case SIGNATURE_TYPE_FLOAT_REFERENCE:
      value = v8::Number::New(isolate,
                              *reinterpret_cast<float*>(&static_buffer_->number_values[argument]));

      if (eager_return)
        return value;

      return_array->Set(context, stored_return_values++, value);
      break;
    case SIGNATURE_TYPE_INT_REFERENCE:
      value = v8::Number::New(isolate, static_buffer_->number_values[argument]);
      if (eager_return)
        return value;

      return_array->Set(context, stored_return_values++, value);
      break;
    case SIGNATURE_TYPE_STRING_REFERENCE:
      {
        v8::MaybeLocal<v8::String> maybe =
            v8::String::NewFromUtf8(isolate,
                                    static_buffer_->string_values[argument], v8::NewStringType::kNormal);

        if (maybe.IsEmpty())
          value = v8::Null(isolate);
        else
          value = maybe.ToLocalChecked();
      }

      if (eager_return)
        return value;

      return_array->Set(context, stored_return_values++, value);
      break;
    }
  }

  return return_array;
}

bool PawnInvoke::ParseSignature(v8::Local<v8::Value> signature,
                                size_t* argument_count, size_t* return_count) {
  v8::String::Utf8Value string(GetIsolate(), signature);
  DCHECK(*string);

  bool found_reference = false;
  for (int index = 0; index < string.length(); ++index) {
    const char type = (*string)[index];
    const bool is_reference = type == 'F' || type == 'I' || type == 'S';

    // Non-reference arguments may not follow reference arguments.
    if (found_reference && !is_reference)
      return false;

    found_reference |= is_reference;

    switch (type) {
    case 'a':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_ARRAY;
      *argument_count += 1;
      break;
    case 'f':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_FLOAT;
      *argument_count += 1;
      break;
    case 'F':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_FLOAT_REFERENCE;
      *return_count += 1;
      break;
    case 'i':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_INT;
      *argument_count += 1;
      break;
    case 'I':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_INT_REFERENCE;
      *return_count += 1;
      break;
    case 's':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_STRING;
      *argument_count += 1;
      break;
    case 'S':
      static_buffer_->signature[index] = SignatureType::SIGNATURE_TYPE_STRING_REFERENCE;
      *return_count += 1;
      break;
    default:
      // The argument type passed in the signature is unknown.
      return false;
    }
  }

  return true;
}

}  // namespace bindings
