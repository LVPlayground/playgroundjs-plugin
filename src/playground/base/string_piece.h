// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copied from strings/stringpiece.h with modifications
//
// A string-like object that points to a sized piece of memory.
//
// You can use StringPiece as a function or method parameter.  A StringPiece
// parameter can receive a double-quoted string literal argument, a "const
// char*" argument, a string argument, or a StringPiece argument with no data
// copying.  Systematic use of StringPiece for arguments reduces data
// copies and strlen() calls.
//
// Prefer passing StringPieces by value:
//   void MyFunction(StringPiece arg);
// If circumstances require, you may also pass by const reference:
//   void MyFunction(const StringPiece& arg);  // not preferred
// Both of these have the same lifetime semantics.  Passing by value
// generates slightly smaller code.  For more discussion, Googlers can see
// the thread go/stringpiecebyvalue on c-users.

#ifndef BASE_STRING_PIECE_H_
#define BASE_STRING_PIECE_H_

#include <stddef.h>

#include <iosfwd>
#include <string>

namespace base {

class StringPiece;

// internal --------------------------------------------------------------------

// Many of the StringPiece functions use different implementations for the
// 8-bit and 16-bit versions, and we don't want lots of template expansions in
// this (very common) header that will slow down compilation.
//
// So here we define overloaded functions called by the StringPiece template.
// For those that share an implementation, the two versions will expand to a
// template internal to the .cc file.
namespace internal {

void CopyToString(const StringPiece& self, std::string* target);
void AppendToString(const StringPiece& self, std::string* target);

size_t copy(const StringPiece& self, char* buf, size_t n, size_t pos);
size_t find(const StringPiece& self, const StringPiece& s, size_t pos);
size_t find(const StringPiece& self, char c, size_t pos);
size_t rfind(const StringPiece& self, const StringPiece& s, size_t pos);
size_t rfind(const StringPiece& self, char c, size_t pos);
size_t find_first_of(const StringPiece& self, const StringPiece& s, size_t pos);
size_t find_first_not_of(const StringPiece& self, const StringPiece& s, size_t pos);
size_t find_first_not_of(const StringPiece& self, char c, size_t pos);
size_t find_last_of(const StringPiece& self, const StringPiece& s, size_t pos);
size_t find_last_of(const StringPiece& self, char c, size_t pos);
size_t find_last_not_of(const StringPiece& self, const StringPiece& s, size_t pos);
size_t find_last_not_of(const StringPiece& self, char c, size_t pos);
StringPiece substr(const StringPiece& self, size_t pos, size_t n);

#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
// Asserts that begin <= end to catch some errors with iterator usage.
void AssertIteratorsInOrder(std::string::const_iterator begin, std::string::const_iterator end);
#endif

}  // namespace internal

class StringPiece {
 public:
  // Standard STL container boilerplate.
  typedef size_t size_type;
  typedef std::string::value_type value_type;
  typedef const value_type* pointer;
  typedef const value_type& reference;
  typedef const value_type& const_reference;
  typedef ptrdiff_t difference_type;
  typedef const value_type* const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  static const size_type npos;

 public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected (likewise for char16, string16, StringPiece16).
  StringPiece() : ptr_(NULL), length_(0) {}
  StringPiece(const value_type* str)
      : ptr_(str),
        length_((str == NULL) ? 0 : std::string::traits_type::length(str)) {}
  StringPiece(const std::string& str)
      : ptr_(str.data()), length_(str.size()) {}
  StringPiece(const value_type* offset, size_type len)
      : ptr_(offset), length_(len) {}
  StringPiece(const std::string::const_iterator& begin,
              const std::string::const_iterator& end) {
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
    // This assertion is done out-of-line to avoid bringing in logging.h and
    // instantiating logging macros for every instantiation.
    internal::AssertIteratorsInOrder(begin, end);
#endif
    length_ = static_cast<size_t>(std::distance(begin, end));

    // The length test before assignment is to avoid dereferencing an iterator
    // that may point to the end() of a string.
    ptr_ = length_ > 0 ? &*begin : nullptr;
  }

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.
  const value_type* data() const { return ptr_; }
  size_type size() const { return length_; }
  size_type length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }
  void set(const value_type* data, size_type len) {
    ptr_ = data;
    length_ = len;
  }
  void set(const value_type* str) {
    ptr_ = str;
    length_ = str ? std::string::traits_type::length(str) : 0;
  }

  value_type operator[](size_type i) const { return ptr_[i]; }

  void remove_prefix(size_type n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_type n) {
    length_ -= n;
  }

  int compare(const StringPiece& x) const {
    int r = wordmemcmp(
      ptr_, x.ptr_, (length_ < x.length_ ? length_ : x.length_));
    if (r == 0) {
      if (length_ < x.length_) r = -1;
      else if (length_ > x.length_) r = +1;
    }
    return r;
  }

  std::string as_string() const {
    // std::string doesn't like to take a NULL pointer even with a 0 size.
    return empty() ? std::string() : std::string(data(), size());
  }

  const_iterator begin() const { return ptr_; }
  const_iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }

  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  static int wordmemcmp(const value_type* p,
    const value_type* p2,
    size_type N) {
    return std::string::traits_type::compare(p, p2, N);
  }

  // Sets the value of the given string target type to be the current string.
  // This saves a temporary over doing |a = b.as_string()|
  void CopyToString(std::string* target) const {
    internal::CopyToString(*this, target);
  }

  void AppendToString(std::string* target) const {
    internal::AppendToString(*this, target);
  }

  size_type copy(value_type* buf, size_type n, size_type pos = 0) const {
    return internal::copy(*this, buf, n, pos);
  }

  // Does "this" start with "x"
  bool starts_with(const StringPiece& x) const {
    return ((this->length_ >= x.length_) &&
      (wordmemcmp(this->ptr_, x.ptr_, x.length_) == 0));
  }

  // Does "this" end with "x"
  bool ends_with(const StringPiece& x) const {
    return ((this->length_ >= x.length_) &&
      (wordmemcmp(this->ptr_ + (this->length_ - x.length_),
        x.ptr_, x.length_) == 0));
  }

  // find: Search for a character or substring at a given offset.
  size_type find(const StringPiece& s, size_type pos = 0) const {
    return internal::find(*this, s, pos);
  }
  size_type find(value_type c, size_type pos = 0) const {
    return internal::find(*this, c, pos);
  }

  // rfind: Reverse find.
  size_type rfind(const StringPiece& s, size_type pos = StringPiece::npos) const {
    return internal::rfind(*this, s, pos);
  }
  size_type rfind(value_type c, size_type pos = StringPiece::npos) const {
    return internal::rfind(*this, c, pos);
  }

  // find_first_of: Find the first occurence of one of a set of characters.
  size_type find_first_of(const StringPiece& s, size_type pos = 0) const {
    return internal::find_first_of(*this, s, pos);
  }
  size_type find_first_of(value_type c, size_type pos = 0) const {
    return find(c, pos);
  }

  // find_first_not_of: Find the first occurence not of a set of characters.
  size_type find_first_not_of(const StringPiece& s, size_type pos = 0) const {
    return internal::find_first_not_of(*this, s, pos);
  }
  size_type find_first_not_of(value_type c, size_type pos = 0) const {
    return internal::find_first_not_of(*this, c, pos);
  }

  // find_last_of: Find the last occurence of one of a set of characters.
  size_type find_last_of(const StringPiece& s, size_type pos = StringPiece::npos) const {
    return internal::find_last_of(*this, s, pos);
  }
  size_type find_last_of(value_type c, size_type pos = StringPiece::npos) const {
    return rfind(c, pos);
  }

  // find_last_not_of: Find the last occurence not of a set of characters.
  size_type find_last_not_of(const StringPiece& s, size_type pos = StringPiece::npos) const {
    return internal::find_last_not_of(*this, s, pos);
  }
  size_type find_last_not_of(value_type c, size_type pos = StringPiece::npos) const {
    return internal::find_last_not_of(*this, c, pos);
  }

  // substr.
  StringPiece substr(size_type pos, size_type n = StringPiece::npos) const {
    return internal::substr(*this, pos, n);
  }

 protected:
  const value_type* ptr_;
  size_type         length_;
};

// StingPiece operators --------------------------------------------------------

bool operator==(const StringPiece& x, const StringPiece& y);

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
  const int r = StringPiece::wordmemcmp(
      x.data(), y.data(), (x.size() < y.size() ? x.size() : y.size()));
  return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
  return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
  return !(x < y);
}

std::ostream& operator<<(std::ostream& o, const StringPiece& piece);

}  // namespace base

#endif  // BASE_STRING_PIECE_H_
