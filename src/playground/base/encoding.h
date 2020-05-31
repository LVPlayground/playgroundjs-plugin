// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_ENCODING_H_
#define PLAYGROUND_BASE_ENCODING_H_

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <codecvt>
#include <string>

// Converts the given |ansiString| to an UTF-8 string with, optionally, the given |locale|.
std::string fromAnsi(const std::string& ansiString, const std::locale& locale = std::locale{});

// Converts the given |utf8String| to an ANSI string with, optionally, the given |locale|.
std::string toAnsi(const char* utf8String, size_t length, const std::locale& locale = std::locale{});
std::string toAnsi(const std::string& utf8String, const std::locale& locale = std::locale{});

#endif  // PLAYGROUND_BASE_ENCODING_H_
