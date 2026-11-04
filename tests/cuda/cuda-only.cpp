// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_cuda -DCUDA_TEST
// Balanced:
// RUN: %load_cpt_cuda %cpt_options_cuda %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Orchestration Efficiency:
// RUN: %load_cpt_cuda %cpt_options_cuda %t 10. 20. 0. 1000 1000 1000 500 | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Device Offload Efficiency:
// the host only waits for the device
// RUN: %load_cpt_cuda %cpt_options_cuda %t 10. 0. 0. 0 1000 1000 1000 | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Device Communication Efficiency:
// the kernel is done when the memory transfer starts, the host is only waiting
// during the transfer, which can overrun by one copy (slightly below 500)
// RUN: %load_cpt_cuda %cpt_options_cuda %t 10. 10. 10. 495 500 1000 1000 | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Kernel and Memory Transfer Overlap:
// a memory transfer during the kernel of its own stream results in ~0s mt time,
// it stays shorter than the kernel as it can overrun by one copy
// RUN: %load_cpt_cuda %cpt_options_cuda %t 10. 0. 8. 0 1000 1000 1000 | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile

// clang-format on
// REQUIRES: CUDA

#include "cuda-common.h"
#include "expected-metrics.h"

struct TestParams {
  double workAsync{10.};
  double workHost{10.};
  double memTransfer{0.};
};

int main(int argc, char **argv) {
  TestParams params;
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
  if (argc == 8) {
    params.workAsync = atof(argv[1]);
    params.workHost = atof(argv[2]);
    params.memTransfer = atof(argv[3]);
    m.HDE = atoi(argv[4]);
    m.DCE = atoi(argv[5]);
    m.DLB = atoi(argv[6]);
    m.DOE = atoi(argv[7]);
  }

  printMetrics(m);

  InitDevices();
  EnsureNStreams(0, 1);

  BeginTest();
  DoWorkAsync(WORK * params.workAsync, 0, dtcs[0].streams[0]);
  usleep(WORK * params.workHost);
  DoMemTransfer(WORK * params.memTransfer, 1);

  DeviceSynchronize();
  EndTest();

  CleanUpDevices();
}