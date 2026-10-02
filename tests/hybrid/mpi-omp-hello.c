// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_mpi_omp_c
// RUN: %mpirunnp 2 env %load_cpt CPT_OPTIONS="barrier=1 metric_factor=1000" %t | %FileCheck \
// RUN: --check-prefixes=CHECK %metricfile
// RUN: %mpirunnp 2 env %load_cpt CPT_OPTIONS="barrier=1 start_with_library_constructor=1 metric_factor=1000" %t | %FileCheck \
// RUN: --check-prefixes=CHECK %metricfile
// clang-format on

#include <mpi.h>
#include <omp.h>
#include <unistd.h>

#undef LB_FAK
#undef UB_FAK
#define LB_FAK 0.5
#define UB_FAK 1.01

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int threads = omp_get_max_threads();

  metrics m = {1000, 1000, 1000, 1000, 1000, 1000};
  if (rank == 0) {
    printMetrics(m);
  }

  usleep(5 * WORK);

  printf("Hello, I am %d of %d tasks. Max threads reported %d\n", rank, size,
         threads);

  MPI_Finalize();

  return 0;
}
