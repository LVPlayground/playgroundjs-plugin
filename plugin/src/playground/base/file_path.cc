// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "base/file_path.h"

#include <algorithm>

#if defined(WIN32)
#include <windows.h>
#undef max
#else
#include <unistd.h>
#endif

namespace base {

namespace {

FilePath current_directory_;

#if defined(WIN32)
const char kSeparators[] = "\\/";
const size_t kSeparatorsLength = 2;
#else
const char kSeparators[] = "/";
const size_t kSeparatorsLength = 1;
#endif

bool IsSeparator(char character) {
  for (int index = 0; index < kSeparatorsLength; ++index)
    if (kSeparators[index] == character)
      return true;
  return false;
}

size_t FindDriveLetter(const std::string& path) {
#if defined(WIN32)
  if (path.length() >= 2 && path[1] == L':' &&
      ((path[0] >= L'A' && path[0] <= L'Z') ||
       (path[0] >= L'a' && path[0] <= L'z'))) {
    return 1;
  }
#endif  // OS_WINDOWS

  return std::string::npos;
}

}  // namespace

// -----------------------------------------------------------------------------

void FilePath::Initialize() {
#if defined(WIN32)
  char directory_buffer[MAX_PATH];

  GetCurrentDirectoryA(MAX_PATH, directory_buffer);
  current_directory_ = FilePath(std::string(directory_buffer));
#else
  char directory_buffer[FILENAME_MAX];

  getcwd(directory_buffer, FILENAME_MAX);
  current_directory_ = FilePath(std::string(directory_buffer));
#endif
}

FilePath FilePath::CurrentDirectory() {
  return current_directory_;
}

// -----------------------------------------------------------------------------

FilePath::FilePath() {}

FilePath::FilePath(const FilePath& path)
    : path_(path.path_) {}

FilePath::FilePath(const std::string& path)
    : path_(path) {}

// -----------------------------------------------------------------------------

FilePath FilePath::DirName() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparators();

  const size_t letter = FindDriveLetter(new_path.path_);
  const size_t last_separator = new_path.path_.find_last_of(kSeparators,
    std::string::npos, kSeparatorsLength);

  if (last_separator == std::string::npos)
    new_path.path_.resize(letter + 1);
  else
    new_path.path_.resize(last_separator);

  if (!new_path.path_.length())
    new_path.path_ = current_directory_.path_;

  new_path.StripTrailingSeparators();
  return new_path;
}

FilePath FilePath::BaseName() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparators();

  const size_t letter = FindDriveLetter(new_path.path_);
  if (letter != std::string::npos)
    new_path.path_.erase(0, letter + 1);

  const size_t last_separator = new_path.path_.find_last_of(kSeparators,
    std::string::npos, kSeparatorsLength);
  if (last_separator != std::string::npos &&
    last_separator < new_path.path_.length() - 1) {
    new_path.path_.erase(0, last_separator + 1);
  }

  return new_path;
}

FilePath FilePath::Append(const std::string& component) const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparators();

  if (new_path.path_.length() > 0 && component.length() > 0) {
    if (!IsSeparator(new_path.path_[new_path.path_.length() - 1]) &&
      !IsSeparator(component[0]))
      new_path.path_.append(1, kSeparators[0]);
  }

  new_path.path_.append(component);
  return new_path;
}

FilePath FilePath::Append(const FilePath& component) const {
  return Append(component.value());
}

bool FilePath::IsAbsolute() const {
#if defined(WIN32)
  const size_t drive_letter = FindDriveLetter(path_);
  if (drive_letter != std::string::npos) {
    return path_.length() > drive_letter + 1 &&
      IsSeparator(path_[drive_letter + 1]);
  }

  return path_.length() > 1 && IsSeparator(path_[0]) &&
    IsSeparator(path_[1]);
#else
  return path_.length() > 0 && IsSeparator(path_[0]);
#endif
}

FilePath FilePath::EnsureAbsolute() const {
  if (!IsAbsolute())
    return FilePath::CurrentDirectory().Append(*this);

  return *this;
}

// -----------------------------------------------------------------------------

void FilePath::StripTrailingSeparators() {
  const size_t last_character = path_.find_last_not_of(kSeparators,
    std::string::npos, kSeparatorsLength);
  path_.resize(std::max(last_character + 1, 1u));
}

}  // namespace base
