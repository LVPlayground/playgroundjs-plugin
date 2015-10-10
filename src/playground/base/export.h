// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_EXPORT_H_
#define PLAYGROUND_BASE_EXPORT_H_

#if defined(WIN32)

#if defined(PLAYGROUND_IMPLEMENTATION)
#define PLAYGROUND_EXPORT __declspec(dllexport)
#else
#define PLAYGROUND_EXPORT __declspec(dllimport)
#endif

#else  // defined(WIN32)

#if defined(PLAYGROUND_IMPLEMENTATION)
#define PLAYGROUND_EXPORT __attribute__((visibility("default")))
#else
#define PLAYGROUND_EXPORT
#endif

#endif

#endif  // PLAYGROUND_BASE_EXPORT_H_
