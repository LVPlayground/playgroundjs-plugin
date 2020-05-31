// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/encoding.h"
#include "base/logging.h"

#include <unicode/ucnv.h>

// Default string buffer size. The buffer is able to resize itself when this is necessary for the
// data that is being converted. It will never scale back down.
const size_t kDefaultBufferSize = 1024;

UConverter* g_platform_converter = nullptr;
UConverter* g_unicode_converter = nullptr;

std::string* g_conversion_buffer = nullptr;

namespace {

// Converts the |input| (being |length| characters long) from |from| to |to|.
std::string convert(UConverter* from, UConverter* to, const char* input, size_t length) {
  const size_t required_buffer_size = static_cast<size_t>(ceil(length * 1.25));
  if (g_conversion_buffer->capacity() < required_buffer_size)
    g_conversion_buffer->resize(required_buffer_size, '\0');

  char* buffer = &(*g_conversion_buffer)[0];
  char* target = buffer;
  
  UErrorCode error = U_ZERO_ERROR;
  ucnv_convertEx(to, from,
    &target, target + g_conversion_buffer->capacity(),
    &input, input + length,
    nullptr, nullptr, nullptr, nullptr,
    /* reset= */ true, /* flush= */ true, &error);

  if (U_FAILURE(error)) {
    UErrorCode name_error = U_ZERO_ERROR;
    LOG(WARNING) << "Unable to convert string from " << ucnv_getName(from, &name_error) << " to "
                 << ucnv_getName(to, &name_error) << ": ddfdf";

    return "";
  }

  std::string result((target - buffer) + 1, '\0');
  result.insert(result.begin(), buffer, target);

  return result;
}

}  // namespace

void initializeEncoding() {
  UErrorCode error = U_ZERO_ERROR;

  g_platform_converter = ucnv_open(NULL, &error);
  CHECK(U_SUCCESS(error)) << "Unable to create the ICU Unicode converter";

  g_unicode_converter = ucnv_open("UTF8", &error);
  CHECK(U_SUCCESS(error)) << "Unable to create the ICU Unicode converter";

  //
  g_conversion_buffer = new std::string(kDefaultBufferSize, '\0');
}

std::string fromAnsi(const char* ansiString, size_t length) {
  return convert(g_platform_converter, g_unicode_converter, ansiString, length);
}

std::string fromAnsi(const std::string& ansiString) {
  return fromAnsi(ansiString.c_str(), ansiString.size());
}

std::string toAnsi(const char* utf8String, size_t length) {
  return convert(g_unicode_converter, g_platform_converter, utf8String, length);
}

std::string toAnsi(const std::string& utf8String) {
  return toAnsi(utf8String.c_str(), utf8String.size());
}
