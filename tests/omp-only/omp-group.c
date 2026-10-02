// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK4,CHECK %metricfile
// RUN: env OMP_NUM_THREADS=2 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK4,CHECK %metricfile
// clang-format on

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int sum = 0, nt = omp_get_max_threads();
  // Expected distribution of work based on libomp tasking implementation:
  // (8+7+6+5+4+3+2+1), (8+7,1+2+3+4+5+6), (8+7,1+3+5,2+4+6), (8,1+4+7,2+5,3+6)
  // locally pop as LIFO, remote steal as FIFO
  int lbs[] = {1000, 18000 / 21, 12000 / 15, 9000 / 12};
  metrics m = {lbs[nt - 1], 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp master
#pragma omp taskgroup
#pragma omp taskgroup
    for (int i = 0; i < 8; i++) {
#pragma omp task
      {
        printf("Thread %i executing Task %i\n", omp_get_thread_num(), i);
        usleep(WORK * (1 + i));
      }
    }
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
  printf("sum = %i\n", sum);
}
