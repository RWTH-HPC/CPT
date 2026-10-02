// ALLOW_RETRIES: 1
// RUN: %compile_mpi_c
// RUN: %mpirunnp 2 %load_cpt %cpt_options_dump_stopped %t | \
// RUN: %FileCheck --check-prefixes=CHECK2,CHECK %metricfile
// RUN: %mpirunnp 4 %load_cpt %cpt_options_dump_stopped %t | \
// RUN: %FileCheck --check-prefixes=CHECK4,CHECK %metricfile

#include <mpi.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int sum = 0, rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  metrics m = {1000, 1000, 1000, 1000 * size / (2 * size - 1), 1000, 1000};
  if (rank == 0) {
    printMetrics(m);
  }
  MPI_Pcontrol(1);
  usleep(WORK * (1 + 2 * rank));
  MPI_Pcontrol(0);
  MPI_Finalize();
}

// CHECK2: Parallel Efficiency:                [[PE:0.6[67]]]{{[0-9]+}}
// CHECK4: Parallel Efficiency:                [[PE:0.57]]{{[0-9]+}}
// CHECK2:   Load Balance:                     [[LB:0.6[67][0-9]+]]
// CHECK4:   Load Balance:                     [[LB:0.57[0-9]+]]
// CHECK:   Communication Efficiency:         [[CE:(1.0|0.99)]]{{[0-9]+}}
// CHECK:     Serialisation Efficiency:       1.000
// CHECK:     Transfer Efficiency:            [[TE:(1.0|0.99)]]{{[0-9]+}}
// CHECK:   MPI Parallel Efficiency:          [[PE]]
// CHECK:     MPI Load Balance:               [[LB]]
// CHECK:     MPI Communication Efficiency:   [[CE]]
// CHECK:       MPI Serialisation Efficiency: 1.000
// CHECK:       MPI Transfer Efficiency:      [[TE]]
// CHECK:   OMP Parallel Efficiency:          {{(0.99[0-9]+|1.00[0-9]+)}}
// CHECK:     OMP Load Balance:               1.000
// CHECK:     OMP Communication Efficiency:   {{(0.99[0-9]+|1.00[0-9]+)}}
// CHECK:       OMP Serialisation Efficiency: 1.000
// CHECK:       OMP Transfer Efficiency:      {{(0.99[0-9]+|1.00[0-9]+)}}
