// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c_delay
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=3 %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=6 %metricfile
// RUN: env OMP_NUM_THREADS=2 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=3 %metricfile
// RUN: env OMP_NUM_THREADS=4 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=6 %metricfile
// clang-format on

// UNSUPPORTED: GNU

#include "delay.h"
#include "expected-metrics.h"
#include <omp.h>
#include <stdio.h>
#include <sys/time.h>

#define NTASKS_TO_THREADS_FACTOR 1.5
#define DEFAULT_DELAY_TIME 5000000 // Default delaytime in microseconds

int main() {

  /* Initialization */
  double delaylength = DEFAULT_DELAY_TIME;

  int nthreads = omp_get_max_threads();
  double factor = NTASKS_TO_THREADS_FACTOR;
  int ntasks = (int)(nthreads * factor);
  factor = ntasks / (double)nthreads;
  int loadbal = factor / ceil(factor) * 1000;
  double cp = ceil(factor) * (delaylength / 1000000);
  printf("%f\n", cp);
  metrics m = {loadbal, 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);

#pragma omp parallel
  {
#pragma omp single
    {
      int i;
      for (i = 0; i < ntasks; i++) {
#pragma omp task
        { delay_sleep(delaylength); }
      }
    }
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}