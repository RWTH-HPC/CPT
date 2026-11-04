// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_cuda_mpi -DCUDA_TEST
// RUN: %mpirunnp 1 %load_cpt_cuda %cpt_options_cuda %t | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile

// clang-format on
// REQUIRES: CUDA

// CUDA-MPI sanity check

#include <mpi.h>

#include "cuda-common.h"
#include "expected-metrics.h"

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};

  printMetrics(m);

  InitDevices();

  MPI_Pcontrol(1);

  DoWorkAsync(WORK * 10.);
  usleep(WORK * 10.);

  DeviceSynchronize();
  MPI_Pcontrol(0);
  CleanUpDevices();
  MPI_Finalize();
}
