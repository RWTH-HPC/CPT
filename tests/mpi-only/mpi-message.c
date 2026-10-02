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
  metrics m = {1000, 1000, 1000, 1000, 1000 / size, 1000};
  if (rank == 0) {
    printMetrics(m);
  }
  MPI_Pcontrol(1);

  if (rank > 0) {
    MPI_Message msg = MPI_MESSAGE_NULL;
    MPI_Status status;
    MPI_Request req = MPI_REQUEST_NULL;
    MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msg, &status);
    MPI_Imrecv(&sum, 1, MPI_INT, &msg, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
  }
  usleep(WORK);

  MPI_Send(&sum, 1, MPI_INT, (rank + 1) % size, 42, MPI_COMM_WORLD);
  {
    MPI_Message msg = MPI_MESSAGE_NULL;
    MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msg,
               MPI_STATUS_IGNORE);
    MPI_Mrecv(&sum, 1, MPI_INT, &msg, MPI_STATUS_IGNORE);
  }
  usleep(WORK);

#if 0 // polling introduces execution time
  MPI_Send(&sum, 1, MPI_INT, (rank + 1) % size, 42, MPI_COMM_WORLD);
  {
    MPI_Message msg = MPI_MESSAGE_NULL;
    MPI_Status status;
    MPI_Request req = MPI_REQUEST_NULL;
    int flag = 0;
    while (!flag)
      MPI_Improbe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &msg,
                  &status);
    MPI_Imrecv(&sum, 1, MPI_INT, &msg, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
  }
  usleep(WORK);

  MPI_Send(&sum, 1, MPI_INT, (rank + 1) % size, 42, MPI_COMM_WORLD);
  {
    MPI_Message msg = MPI_MESSAGE_NULL;
    int flag = 0;
    while (!flag)
      MPI_Improbe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &msg,
                  MPI_STATUS_IGNORE);
    MPI_Mrecv(&sum, 1, MPI_INT, &msg, MPI_STATUS_IGNORE);
  }
  usleep(WORK);
#endif

  if (rank < size - 1) {
    MPI_Send(&sum, 1, MPI_INT, rank + 1, 42, MPI_COMM_WORLD);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Pcontrol(0);

  MPI_Finalize();
}

// CHECK4: Parallel Efficiency:                [[PE:0.2[45]]]{{[0-9]+}}
// CHECK2: Parallel Efficiency:                [[PE:0.(49|50)]]{{[0-9]+}}
// CHECK:   Load Balance:                     [[LB:(1.0|0.99)[0-9]+]]
// CHECK4:   Communication Efficiency:         [[CE:0.2[45]]]{{[0-9]+}}
// CHECK2:   Communication Efficiency:         [[CE:0.(49|50)]]{{[0-9]+}}
// CHECK4:     Serialisation Efficiency:       [[SE:0.25]]{{[0-9]+}}
// CHECK2:     Serialisation Efficiency:       [[SE:0.50]]{{[0-9]+}}
// CHECK:     Transfer Efficiency:            [[TE:(1.0|0.99)]]{{[0-9]+}}
// CHECK:   MPI Parallel Efficiency:          [[PE]]
// CHECK:     MPI Load Balance:               [[LB]]
// CHECK:     MPI Communication Efficiency:   [[CE]]
// CHECK:       MPI Serialisation Efficiency: [[SE]]
// CHECK:       MPI Transfer Efficiency:      [[TE]]
// CHECK:   OMP Parallel Efficiency:          {{(0.99[0-9]+|1.00[0-9]+)}}
// CHECK:     OMP Load Balance:               1.000
// CHECK:     OMP Communication Efficiency:   {{(0.99[0-9]+|1.00[0-9]+)}}
// CHECK:       OMP Serialisation Efficiency: 1.000
// CHECK:       OMP Transfer Efficiency:      {{(0.99[0-9]+|1.00[0-9]+)}}
