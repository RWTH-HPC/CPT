// clang-format off
// RUN: %compile_mpi_c
// ALLOW_RETRIES: 1
// RUN: %mpirunnp 2 %load_cpt CPT_OPTIONS="barrier=1 metric_factor=1000" %t | %FileCheck \
// RUN: --check-prefixes=CHECK %metricfile
// RUN: %mpirunnp 2 %load_cpt CPT_OPTIONS="barrier=1 start_with_library_constructor=1 metric_factor=1000" %t | %FileCheck \
// RUN: --check-prefixes=CHECK %metricfile
// clang-format on

#include <mpi.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000};
  if (rank == 0) {
    printMetrics(m);
  }

  usleep(WORK);

  printf("Hello, I am %d of %d tasks.\n", rank, size);

  MPI_Finalize();

  return 0;
}
