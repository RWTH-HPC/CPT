// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_cuda -DCUDA_TEST
// RUN: %load_cpt_cuda %cpt_options_cuda %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// RUN: %load_cpt_cuda CPT_OPTIONS="stopped=1 dump_on_stop=1 cuda_analysis=1 verbose=1" %t | \
// RUN: %FileCheck --check-prefix=OVERLAP %s

// two concurrent kernels of 1s on separate streams, synchronized per stream
// only, on top of 1s of work outside of the measurement.
// OVERLAP: Number of Devices: {{ *}}1
// OVERLAP: Average Device Kernel Time (in s): {{ *(0\.9[5-9]|1\.[01])}}
// OVERLAP: Average Device Idle Time (in s): {{ *0\.[01]}}
// OVERLAP: Average Kernel Overlap (in s): {{ *(0\.9|1\.[01])}}
// OVERLAP: Average Transfer Overlap (in s): {{ *0\.000}}

// clang-format on
// REQUIRES: CUDA

#include "cuda-common.h"
#include "expected-metrics.h"

int main(int argc, char **argv) {
  // the host only waits for the device
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000, 0, 1000, 1000, 1000};
  printMetrics(m);

  InitDevices();
  EnsureNStreams(0, 2);

  DoWork(10 * WORK);

  BeginTest();
  DoWorkNStreamAsync(10 * WORK, 0, 2);
  SyncWorkStreams(0);
  EndTest();

  CleanUpDevices();
}
