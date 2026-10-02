// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK %metricfile
// RUN: env OMP_NUM_THREADS=2 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK %metricfile
// clang-format on

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int sum = 0, nt = omp_get_max_threads();
  metrics m = {1000, 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel for schedule(static, 1) reduction(+ : sum)
  for (int i = 0; i < 20; i++) {
    sum += i;
    usleep(WORK);
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);

  printf("sum = %i\n", sum);
}
