// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_FILE_SEARCH_H_
#define PLAYGROUND_BASE_FILE_SEARCH_H_

#include <string>
#include <vector>

#include "base/file_path.h"

enum class FileSearchStatus {
  SUCCESS,
  ERR_INVALID_REGEX
};

FileSearchStatus FileSearch(const base::FilePath& base, const std::string& query, std::vector<std::string>* results);

#endif  // PLAYGROUND_BASE_FILE_SEARCH_H_
