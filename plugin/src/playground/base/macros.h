// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_MACROS_H_
#define PLAYGROUND_BASE_MACROS_H_

// A macro to disallow the copy constructor and operator= functions.
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName)   \
    TypeName(const TypeName&) = delete;      \
    void operator=(const TypeName&) = delete

#endif  // PLAYGROUND_BASE_MACROS_H_
