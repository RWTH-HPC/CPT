// ALLOW_RETRIES: 5
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %metricfile
// RUN: env OMP_NUM_THREADS=2 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %metricfile

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#include "expected-metrics.h"

#define delay(t) usleep(t);

#define OMPT_SIGNAL(s) ompt_signal(&s)
// inline
void ompt_signal(int *s) {
#pragma omp atomic
  (*s)++;
}

#define OMPT_WAIT(s, v) ompt_wait(&s, v)
// wait for s >= v
// inline
void ompt_wait(int *s, int v) {
  int wait = 0;
  do {
    delay(10);
#pragma omp atomic read
    wait = (*s);
  } while (wait < v);
}

int main(int argc, char **argv) {
  int condition = 0, nt = omp_get_max_threads();
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
    usleep(100000);
#pragma omp master
    {
      for (int i = 0; i < 4; i++) {
#pragma omp task shared(condition)
        {
          int tid = omp_get_thread_num();
          usleep(100000);
          // printf("Hello from thread = %i\n", tid);
          // OMPT_SIGNAL(condition);
        }
      }
      // OMPT_WAIT(condition,10);
    }
#pragma omp barrier
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}

// CHECK2: Parallel Efficiency:                [[PE:0.9(9|8)[0-9]+]]
// CHECK2:   Load Balance:                     [[LB:0.9(9|8)[0-9]+]]
// CHECK:   Communication Efficiency:         [[CE:0.99[0-9]+]]
// CHECK:     Serialisation Efficiency:       [[SE:(1.0|0.99)[0-9]+]]
// CHECK:     Transfer Efficiency:            [[TE:0.99[0-9]+]]
// CHECK:   MPI Parallel Efficiency:          1.000000
// CHECK:     MPI Load Balance:               1.000000
// CHECK:     MPI Communication Efficiency:   1.000000
// CHECK:       MPI Serialisation Efficiency: 1.000000
// CHECK:       MPI Transfer Efficiency:      1.000000
// CHECK:   OMP Parallel Efficiency:          [[PE]]
// CHECK:     OMP Load Balance:               [[LB]]
// CHECK:     OMP Communication Efficiency:   [[CE]]
// CHECK:       OMP Serialisation Efficiency: [[SE]]
// CHECK:       OMP Transfer Efficiency:      [[TE]]
// CHECK: ##### FROM HERE FIBER VALUES #####
// CHECK2: Parallel Efficiency:                [[PE:0.(5|49)[0-9]+]]
// CHECK2:   Load Balance:                     [[LB:0.(5|49)[0-9]+]]
// CHECK:   Communication Efficiency:         [[CE:0.99[0-9]+]]
// CHECK:     Serialisation Efficiency:       [[SE:(1.0|0.99)[0-9]+]]
// CHECK:     Transfer Efficiency:            [[TE:0.99[0-9]+]]
// CHECK:   MPI Parallel Efficiency:          1.000000
// CHECK:     MPI Load Balance:               1.000000
// CHECK:     MPI Communication Efficiency:   1.000000
// CHECK:       MPI Serialisation Efficiency: 1.000000
// CHECK:       MPI Transfer Efficiency:      1.000000
// CHECK:   OMP Parallel Efficiency:          [[PE]]
// CHECK:     OMP Load Balance:               [[LB]]
// CHECK:     OMP Communication Efficiency:   [[CE]]
// CHECK:       OMP Serialisation Efficiency: [[SE]]
// CHECK:       OMP Transfer Efficiency:      [[TE]]
