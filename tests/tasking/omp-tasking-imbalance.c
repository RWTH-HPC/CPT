// ALLOW_RETRIES: 5
// clang-format off
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK,CHECK-TASK -D#ETCR=8 %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK4,CHECK,CHECK-TASK -D#ETCR=8 %metricfile
// RUN: env OMP_NUM_THREADS=2 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK,CHECK-TASK -D#ETCR=8 %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK4,CHECK,CHECK-TASK -D#ETCR=8 %metricfile

// UNSUPPORTED: GNU
// clang-format on

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#undef LB_FAK
#undef UB_FAK
#define LB_FAK 0.8
#define UB_FAK 1.2

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int sum = 0, nt = omp_get_max_threads();
  // total: t=36
  // 1 Thread: {1..8}: 36/36 = 100%
  // 2 Threads: {8,7,6},{1..5}: 18/21 = 87%
  // 3 Threads: {8,7},{1,3,5},{2,4,6}: 12/15 = 80%
  // 4 Threads: {8},{1,4,7},{2,5},{3,6}: 9/12 = 75%
  int ib[] = {1000, 1000 * 18 / 21, 800, 750};
  metrics m = {ib[nt - 1], 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp master
    for (int i = 0; i < 8; i++) {
#pragma omp task
      {
        printf("Thread %i executing Task %i\n", omp_get_thread_num(), i);
        usleep(WORK * (1 + i));
      }
    }
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}
