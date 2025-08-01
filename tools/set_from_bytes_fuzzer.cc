// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <jxl/memory_manager.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "lib/extras/codec.h"
#include "lib/extras/codec_in_out.h"
#include "lib/extras/size_constraints.h"
#include "lib/jxl/base/common.h"
#include "lib/jxl/base/compiler_specific.h"
#include "lib/jxl/base/span.h"
#include "lib/jxl/base/status.h"
#include "lib/jxl/fuzztest.h"
#include "tools/thread_pool_internal.h"
#include "tools/tracking_memory_manager.h"

namespace {

using ::jpegxl::tools::kGiB;
using ::jpegxl::tools::ThreadPoolInternal;
using ::jpegxl::tools::TrackingMemoryManager;
using ::jxl::Bytes;
using ::jxl::CodecInOut;
using ::jxl::SizeConstraints;
using ::jxl::Status;

void CheckImpl(bool ok, const char* conndition, const char* file, int line) {
  if (!ok) {
    fprintf(stderr, "Check(%s) failed at %s:%d\n", conndition, file, line);
    JXL_CRASH();
  }
}
#define Check(OK) CheckImpl((OK), #OK, __FILE__, __LINE__)

Status Run(const uint8_t* data, size_t size, JxlMemoryManager* memory_manager,
           const SizeConstraints& constraints) {
  auto io = jxl::make_unique<CodecInOut>(memory_manager);
  ThreadPoolInternal pool(0);

  (void)jxl::SetFromBytes(Bytes(data, size), io.get(), pool.get(),
                          &constraints);
  return true;
}

int DoTestOneInput(const uint8_t* data, size_t size) {
  SizeConstraints constraints;
  constraints.dec_max_xsize = 1u << 16;
  constraints.dec_max_ysize = 1u << 16;
  constraints.dec_max_pixels = 1u << 22;

  TrackingMemoryManager memory_manager{/* cap */ 1 * kGiB,
                                       /* total_cap */ 5 * kGiB};
  // It is OK to fail.
  (void)Run(data, size, memory_manager.get(), constraints);
  Check(memory_manager.Reset());

  return 0;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return DoTestOneInput(data, size);
}

void TestOneInput(const std::vector<uint8_t>& data) {
  DoTestOneInput(data.data(), data.size());
}

FUZZ_TEST(SetFromBytesFuzzTest, TestOneInput);
