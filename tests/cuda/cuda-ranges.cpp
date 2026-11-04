// clang-format off
// ALLOW_RETRIES: 1
// Work before and after the range, in ranges of the application:
// RUN: %compile_cuda -DCUDA_TEST -DTEST_BEFORE -DTEST_AFTER
// RUN: %load_cpt_cuda %cpt_options_cuda %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Same, dumping in the library destructor:
// RUN: %load_cpt_cuda CPT_OPTIONS="stopped=1 metric_factor=1000 cuda_analysis=1" %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Ranges of the application overlapping the range and a nested CPT range inside the range:
// RUN: %compile_cuda -DCUDA_TEST -DTEST_INSIDE
// RUN: %load_cpt_cuda %cpt_options_cuda %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// No CUDA call after the range, the library destructor has to dump:
// RUN: %compile_cuda -DCUDA_TEST -DTEST_NO_CLEANUP
// RUN: %load_cpt_cuda %cpt_options_cuda %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// clang-format on
// REQUIRES: CUDA

#include "cuda-common.h"
#include "expected-metrics.h"
#include <nvtx3/nvToolsExt.h>

// the host only waits here, so this kernel lowers HDE if it is ever measured
static void WorkOutsideRange() {
  nvtxRangeId_t app = nvtxRangeStartA("app");
  DoWorkAsync(WORK * 10.);
  DeviceSynchronize();
  nvtxRangeEnd(app);
}

int main(int argc, char **argv) {
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);

  InitDevices();

#ifdef TEST_BEFORE
  WorkOutsideRange();
#endif

#ifdef TEST_INSIDE
  // started before and ended inside the range, its end must not stop the tool
  nvtxRangeId_t overlapping = nvtxRangeStartA("app");
#endif
  BeginTest();
#ifdef TEST_INSIDE
  nvtxRangeEnd(overlapping);
  // ends here must not stop the tool, the balanced work comes afterwards
  nvtxRangeId_t app = nvtxRangeStartA("app");
  nvtxRangeEnd(app);
  nvtxRangeId_t inner = nvtxRangeStartA("CPT inner");
  nvtxRangeEnd(inner);
#endif
  DoWorkAsync(WORK * 10.);
  usleep(WORK * 10.);
  DeviceSynchronize();
  EndTest();

#ifdef TEST_AFTER
  WorkOutsideRange();
#endif

#ifndef TEST_NO_CLEANUP
  CleanUpDevices();
#endif
}
