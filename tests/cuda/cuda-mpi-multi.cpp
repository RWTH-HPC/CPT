// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_cuda_mpi -DCUDA_TEST
// Device and Host Load Balance:
// host is waiting for GPU which causes also host imbalance
// RUN: %mpirunnp 2 %load_cpt_cuda %cpt_options_cuda %t 10. 0. 0. 1. 0 1000 750 1000 750 | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile
// Device Load Balance
// RUN: %mpirunnp 2 %load_cpt_cuda %cpt_options_cuda %t 10. 10. 0. 1. 1000 1000 750 1000 1000 | \
// RUN: %FileCheck --check-prefixes=CHECK,CHECKCUDA %metricfile

// clang-format on
// REQUIRES: CUDA, multi-gpu

#include <mpi.h>

#include "cuda-common.h"
#include "expected-metrics.h"

struct TestParams {
  double workAsync{10.};
  double workHost{10.};
  double memTransfer{0.};
  double imbalance{1.};
};

int main(int argc, char **argv) {
  int sum = 0, rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  TestParams params;
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
  if (argc == 10) {
    params.workAsync = atof(argv[1]);
    params.workHost = atof(argv[2]);
    params.memTransfer = atof(argv[3]);
    params.imbalance = atof(argv[4]);
    m.HDE = atoi(argv[5]);
    m.DCE = atoi(argv[6]);
    m.DLB = atoi(argv[7]);
    m.DOE = atoi(argv[8]);
    m.MLB = atoi(argv[9]);
  }
  if (rank == 0) {
    printMetrics(m);
  }

  if (size > GetNumDevices()) {
    fprintf(stderr,
            "Insufficient number of devices for the number of processes.\n");
    MPI_Finalize();
    return -1;
  }

  InitDevices(1, rank);
  MPI_Pcontrol(1);

  DoWorkAsync(WORK * params.workAsync * params.imbalance * (rank + 1));
  usleep(size * WORK * params.workHost);

  DeviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Pcontrol(0);
  CleanUpDevices();
  MPI_Finalize();
}