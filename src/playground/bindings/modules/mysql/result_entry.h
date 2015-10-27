// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_MODULES_MYSQL_RESULT_ENTRY_H_
#define PLAYGROUND_BINDINGS_MODULES_MYSQL_RESULT_ENTRY_H_

#include <map>
#include <string>
#include <vector>

#ifndef WIN32
#include <cstring>
#include <stdlib.h>
#endif

namespace mysql {

struct ColumnInfo {
  enum ColumnType {
    StringColumnType,
    IntegerColumnType,
    FloatColumnType,
    UnknownColumnType
  };

  ColumnInfo() : name(std::string()), type(StringColumnType) {}
  ColumnInfo(const char* in_name, ColumnType in_type, int in_index)
    : name(in_name)
    , type(in_type)
    , index(in_index)
  {
  }

  std::string name;
  ColumnType type;
  int index;
};

class RowInfo {
 public:
  struct FieldValue {
    FieldValue(const char* value) : string_val(value), owns_string(true) {}
    FieldValue(int value) : integer_val(value), owns_string(false) {}
    FieldValue(float value) : float_val(value), owns_string(false) {}

    bool owns_string;
    union {
      const char* string_val;
      int integer_val;
      float float_val;
    };
  };

 public:
  RowInfo()
    : columns_()
  {
  }

  RowInfo(RowInfo&& other) {
    if (this == &other)
      return; // don't support self assignment.

    columns_.swap(other.columns_);
  }

  RowInfo& operator=(const RowInfo& other) = default;

  ~RowInfo() {
    std::vector<FieldValue>::iterator iter;
    for (iter = columns_.begin(); iter != columns_.end(); ++iter) {
      if (iter->owns_string == false)
        continue;
      delete iter->string_val;
    }
  }

  const FieldValue* column(unsigned int index) {
    if (columns_.size() <= index)
      return 0;

    return &columns_[index];
  }

  void pushField(const char* value, unsigned long length, ColumnInfo::ColumnType type) {
    char* fieldMemory = 0; // as we have to allocate our own strings.

    switch(type) {
    case ColumnInfo::StringColumnType:
      fieldMemory = static_cast<char*>(calloc(1, length + 1));
      if (value)
        strncpy(fieldMemory, value, length);

      columns_.push_back(FieldValue(fieldMemory));
      break;

    case ColumnInfo::IntegerColumnType:
      columns_.push_back(FieldValue(static_cast<int>(value ? atoi(value) : 0)));
      break;

    case ColumnInfo::FloatColumnType:
      columns_.push_back(FieldValue(static_cast<float>(value ? atof(value) : 0.0)));
      break;
    }
  }

 private:
  std::vector<FieldValue> columns_;
};

class ResultEntry {
  friend class ConnectionClient;
public:
  ResultEntry()
    : affected_rows_(0)
    , insert_id_(0)
    , current_row_(-1)
    , columns_()
    , rows_()
  {
  }

  // Add a new column to the entry. Only to be used by the Connection Client.
  void addColumn(const char* name, ColumnInfo::ColumnType type) {
    columns_[std::string(name)] = ColumnInfo(name, type, columns_.size());
  }

  // Add a new row to the entry. Only to be used by the Connection Client.
  RowInfo* createRow() {
    rows_.emplace_back();
    return &rows_.back();
  }

 public:
  // The following methods are to be called from the server thread.
  unsigned int num_rows() {
    return rows_.size();
  }

  bool fetch_row() {
    return ++current_row_ < static_cast<int>(rows_.size());
  }

  int affected_rows() {
    return affected_rows_;
  }

  int insert_id() {
    return insert_id_;
  }

  const RowInfo::FieldValue* fetch_field(const char* fieldName, ColumnInfo::ColumnType& type) {
    if (current_row_ < 0 || current_row_ >= static_cast<int>(rows_.size()))
      return 0;

    std::map<std::string, ColumnInfo>::iterator entry = columns_.find(std::string(fieldName));
    if (entry == columns_.end())
      return 0;

    int index = entry->second.index;
    type = entry->second.type;

    return rows_[current_row_].column(index);
  }

 private:
  int affected_rows_;
  int insert_id_;

  int current_row_;

  std::map<std::string, ColumnInfo> columns_;
  std::vector<RowInfo> rows_;

};

}  // namespace mysql

#endif  // PLAYGROUND_BINDINGS_MODULES_MYSQL_RESULT_ENTRY_H_
