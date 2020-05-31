// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/encoding.h"

std::string fromAnsi(const std::string& ansiString, const std::locale& locale) {
  using wcvt = std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t>;
  std::u32string wstr(ansiString.size(), U'\0');
  std::use_facet<std::ctype<char32_t>>(locale).widen(ansiString.data(), ansiString.data() + ansiString.size(), &wstr[0]);
  return wcvt{}.to_bytes(
    reinterpret_cast<const int32_t*>(wstr.data()),
    reinterpret_cast<const int32_t*>(wstr.data() + wstr.size())
  );
}

std::string toAnsi(const char* utf8String, size_t length, const std::locale& locale) {
  using wcvt = std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>;
  auto wstr = wcvt{}.from_bytes(utf8String, utf8String + length);

  std::string result(wstr.size(), '0');
  std::use_facet<std::ctype<char32_t>>(locale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
  return result;
}

std::string toAnsi(const std::string& utf8String, const std::locale& locale) {
  return toAnsi(utf8String.c_str(), utf8String.size(), locale);
}
