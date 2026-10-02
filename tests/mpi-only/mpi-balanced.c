// ALLOW_RETRIES: 1
// RUN: %compile_mpi_c
// RUN: %mpirunnp 2 %load_cpt %cpt_options_dump_stopped %t | %FileCheck \
// RUN: --check-prefixes=CHECK %metricfile
// RUN: %mpirunnp 4 %load_cpt %cpt_options_dump_stopped %t | %FileCheck \
// RUN: --check-prefixes=CHECK %metricfile

#include <mpi.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int sum = 0, rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000};
  if (rank == 0) {
    printMetrics(m);
  }

  MPI_Pcontrol(1);
  usleep(WORK);
  MPI_Pcontrol(0);
  MPI_Finalize();
}

// CHECK: Parallel Efficiency:                {{(1.00|0.99)[0-9]+}}
// CHECK:   Load Balance:                     [[LB:(1.0|0.99)[0-9]+]]
// CHECK:   Communication Efficiency:         {{(1.00|0.99)[0-9]+}}
// CHECK:     Serialisation Efficiency:       1.000
// CHECK:     Transfer Efficiency:            {{(1.00|0.99)[0-9]+}}
// CHECK:   MPI Parallel Efficiency:          {{(1.00|0.99)[0-9]+}}
// CHECK:     MPI Load Balance:               [[LB]]
// CHECK:     MPI Communication Efficiency:   {{(1.00|0.99)[0-9]+}}
// CHECK:       MPI Serialisation Efficiency: 1.000
// CHECK:       MPI Transfer Efficiency:      {{(1.00|0.99)[0-9]+}}
// CHECK:   OMP Parallel Efficiency:          {{(0.99[0-9]+|1.00[0-9]+)}}
// CHECK:     OMP Load Balance:               1.000
// CHECK:     OMP Communication Efficiency:   {{(0.99[0-9]+|1.00[0-9]+)}}
// CHECK:       OMP Serialisation Efficiency: 1.000
// CHECK:       OMP Transfer Efficiency:      {{(0.99[0-9]+|1.00[0-9]+)}}
