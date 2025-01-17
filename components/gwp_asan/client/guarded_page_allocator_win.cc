// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "components/gwp_asan/client/guarded_page_allocator.h"

#include "base/logging.h"
#include "build/build_config.h"

namespace gwp_asan {
namespace internal {

// TODO(vtsyrklevich): See if the platform-specific memory allocation and
// protection routines can be broken out in base/ and merged with those used for
// PartionAlloc/ProtectedMemory.
void* GuardedPageAllocator::MapRegion() {
  if (void* hint = MapRegionHint())
    if (void* ptr =
            VirtualAlloc(hint, RegionSize(), MEM_RESERVE, PAGE_NOACCESS))
      return ptr;

  return VirtualAlloc(nullptr, RegionSize(), MEM_RESERVE, PAGE_NOACCESS);
}

void GuardedPageAllocator::UnmapRegion() {
  CHECK(state_.pages_base_addr);
  BOOL err = VirtualFree(reinterpret_cast<void*>(state_.pages_base_addr), 0,
                         MEM_RELEASE);
  DPCHECK(err) << "VirtualFree";
  (void)err;
}

void GuardedPageAllocator::MarkPageReadWrite(void* ptr) {
  LPVOID ret = VirtualAlloc(ptr, state_.page_size, MEM_COMMIT, PAGE_READWRITE);
  PCHECK(ret != nullptr) << "VirtualAlloc";
}

void GuardedPageAllocator::MarkPageInaccessible(void* ptr) {
  BOOL err = VirtualFree(ptr, state_.page_size, MEM_DECOMMIT);
  PCHECK(err != 0) << "VirtualFree";
}

}  // namespace internal
}  // namespace gwp_asan
