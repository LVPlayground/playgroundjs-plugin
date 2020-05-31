// Copyright 2020 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_ENCODING_H_
#define PLAYGROUND_BASE_ENCODING_H_

#include <string>

// To be called when encoding behaviour should be initialized.
void initializeEncoding();

// Converts the given |ansiString| to an UTF-8 string with, optionally, the given |locale|.
std::string fromAnsi(const char* ansiString, size_t length);
std::string fromAnsi(const std::string& ansiString);

// Converts the given |utf8String| to an ANSI string with, optionally, the given |locale|.
std::string toAnsi(const char* utf8String, size_t length);
std::string toAnsi(const std::string& utf8String);

#endif  // PLAYGROUND_BASE_ENCODING_H_
