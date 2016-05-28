// Copyright 2016 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/file_search.h"
#include "base/logging.h"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

FileSearchStatus FileSearch(const base::FilePath& base, const std::string& query, std::vector<std::string>* results) {
  DCHECK(results && results->empty());

  boost::regex filter;
  try {
    filter.assign(query);
  } catch (boost::regex_error& error) {
    LOG(ERROR) << "[" << query << "] " << error.what();
    return FileSearchStatus::ERR_INVALID_REGEX;
  }

  boost::filesystem::path base_path(base.value());
  boost::filesystem::recursive_directory_iterator iter(base_path);
  for (const auto& entry : iter) {
    if (!boost::filesystem::is_regular_file(entry.status()))
      continue;  // |iter| contains a directory

    const boost::filesystem::path path = entry.path();
    if (!boost::regex_match(path.string(), filter))
      continue;  // |iter| does not match the |query|

    results->push_back(path.lexically_relative(base_path).string());
  }

  return FileSearchStatus::SUCCESS;
}
