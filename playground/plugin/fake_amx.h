// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_PLUGIN_FAKE_AMX_H_
#define PLAYGROUND_PLUGIN_FAKE_AMX_H_

#include <memory>

#include "plugin/sdk/amx.h"

namespace plugin {

// The FakeAMX class represents a working, but fake scripting environment for a Pawn runtime, which
// means that we don't have to rely on a live gamemode or filterscript for interaction.
class FakeAMX {
 public:
  FakeAMX();
  ~FakeAMX();

  // Returns a pointer to the local initialized AMX instance.
  AMX* amx() { return &amx_; }

  // All stack operations done on the Fake AMX will be scoped, to make sure that we restore the fake
  // runtime to a clean state after every invocation.
  class ScopedStackModifier {
    public:
    explicit ScopedStackModifier(FakeAMX* fake_amx);
    ~ScopedStackModifier();

    cell PushCell(cell value);
    cell PushString(char* string);
    cell PushArray(size_t size);

    void ReadCell(cell address, cell* dest);
    void ReadArray(cell address, char* data, size_t size);

   private:
    cell Allocate(size_t size);

    FakeAMX* fake_amx_;

    cell stored_hea_;
  };

  ScopedStackModifier GetScopedStackModifier();

 private:
  AMX amx_;
  AMX_HEADER amx_header_;

  std::unique_ptr<unsigned char> amx_heap_;
};

}  // namespace plugin

#endif  // PLAYGROUND_PLUGIN_FAKE_AMX_H_