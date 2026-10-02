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

int main(int argc, char **argv) {
  int sum = 0, nt = omp_get_max_threads();
  metrics m = {1000 * 12 / 13, 1000 * 7 / 8, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
    usleep(1 * WORK);
#pragma omp master
    {
      int x = 1;
#pragma omp task shared(x) depend(out : x)
      {
        printf("Thread %i executing Task 1 (5s)\n", omp_get_thread_num());
        usleep(5 * WORK);
      }
#pragma omp task shared(x) depend(in : x)
      {
        printf("Thread %i executing Task 2 (1s)\n", omp_get_thread_num());
        usleep(1 * WORK);
      }
#pragma omp task shared(x) depend(in : x)
      {
        printf("Thread %i executing Task 3 (2s)\n", omp_get_thread_num());
        usleep(2 * WORK);
      }
#pragma omp task
      {
        printf("Thread %i executing Task 4 (3s)\n", omp_get_thread_num());
        usleep(3 * WORK);
      }
    }
#pragma omp barrier
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}
