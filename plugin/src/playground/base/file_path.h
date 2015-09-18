// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BASE_FILE_PATH_H_
#define PLAYGROUND_BASE_FILE_PATH_H_

#include <string>

namespace base {

class FilePath {
public:
  // Initializes the current directory and compiler directory FilePath constants
  // based on the current environment of the LVP Compiler.
  static void Initialize();

  // Returns a FilePath representing the current directory.
  static FilePath CurrentDirectory();

  // Returns a FilePath representing the compiler directory.
  static FilePath CompilerDirectory();

public:
  FilePath();
  FilePath(const FilePath& path);
  explicit FilePath(const std::string& path);

  FilePath& operator=(const FilePath& path) {
    path_ = path.path_;
    return *this;
  }

  bool operator==(const FilePath& path) { return path_ == path.path_; }
  bool operator!=(const FilePath& path) { return path_ != path.path_; }

  const std::string& value() const { return path_; }

  bool empty() const { return path_.empty(); }

  // Returns a FilePath representing the directory name of the path contained by
  // this object, stripping away the filename component.
  FilePath DirName() const;

  // Returns a FilePath representing the filename of the current path.
  FilePath BaseName() const;

  // Appends |component| as a new component to the current path.
  FilePath Append(const std::string& component) const;
  FilePath Append(const FilePath& component) const;

  // Returns whether the path represented by this object is absolute.
  bool IsAbsolute() const;

  // Ensures that the returned path is absolute. If the path represented by this
  // object is not, the current directory's path will be prepended.
  FilePath EnsureAbsolute() const;

private:
  void StripTrailingSeparators();

  std::string path_;
};

}  // namespace base

#endif  // PLAYGROUND_BASE_FILE_PATH_H_
