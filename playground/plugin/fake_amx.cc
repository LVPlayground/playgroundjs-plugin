// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#include "plugin/fake_amx.h"

#include <string>

#include "base/logging.h"

namespace plugin {

namespace {

// Number of cells to allocate as the heapspace for the fake AMX.
const size_t kHeapCellSize = 4096;

}  // namespace

FakeAMX::FakeAMX() {
  amx_heap_.reset(static_cast<unsigned char*>(calloc(1, kHeapCellSize * sizeof(cell))));

  memset(&amx_, 0, sizeof(amx_));
  amx_.base = reinterpret_cast<unsigned char*>(&amx_header_);
  amx_.callback = amx_Callback;
  amx_.data = amx_heap_.get();
  amx_.flags = AMX_FLAG_NTVREG | AMX_FLAG_RELOC;
  amx_.stk = amx_.stp = kHeapCellSize * sizeof(cell);

  memset(&amx_header_, 0, sizeof(amx_header_));
  amx_header_.amx_version = MIN_AMX_VERSION;
  amx_header_.dat =
      reinterpret_cast<cell>(amx_heap_.get()) - reinterpret_cast<cell>(&amx_header_);
  amx_header_.defsize = sizeof(AMX_FUNCSTUB);
  amx_header_.file_version = CUR_FILE_VERSION;
  amx_header_.magic = AMX_MAGIC;
}

FakeAMX::~FakeAMX() {}

FakeAMX::ScopedStackModifier FakeAMX::GetScopedStackModifier() {
  return ScopedStackModifier(this);
}

FakeAMX::ScopedStackModifier::ScopedStackModifier(FakeAMX* fake_amx)
  : fake_amx_(fake_amx),
    stored_hea_(fake_amx_->amx_.hea) {}

FakeAMX::ScopedStackModifier::~ScopedStackModifier() {
  fake_amx_->amx_.hea = stored_hea_;
}

cell FakeAMX::ScopedStackModifier::PushCell(cell value) {
  cell address = Allocate(1);

  reinterpret_cast<cell*>(fake_amx_->amx_heap_.get())[address / sizeof(cell)] = value;
  return address;
}

cell FakeAMX::ScopedStackModifier::PushString(char* string) {
  DCHECK(string);

  size_t length = strlen(string);
  cell address = Allocate(length + 1);

  amx_SetString(reinterpret_cast<cell*>(fake_amx_->amx_heap_.get()) + address,
                string, 0, 0, length);

  return address;
}

cell FakeAMX::ScopedStackModifier::PushArray(size_t size) {
  return Allocate(size);
}

void FakeAMX::ScopedStackModifier::ReadCell(cell address, cell* dest) {
  *dest = reinterpret_cast<cell*>(fake_amx_->amx_heap_.get())[address / sizeof(cell)];
}

void FakeAMX::ScopedStackModifier::ReadArray(cell address, char* data, size_t size) {
  const cell* source = reinterpret_cast<cell*>(fake_amx_->amx_heap_.get()) + address;
  amx_GetString(data, source, 0, size);
}

cell FakeAMX::ScopedStackModifier::Allocate(size_t size) {
  DCHECK(size > 0);

  cell old_hea = fake_amx_->amx_.hea;
  fake_amx_->amx_.hea += size * sizeof(cell);

  return old_hea;
}

}  // namespace plugin
