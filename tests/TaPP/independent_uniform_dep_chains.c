// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c_delay
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=1500 %combinedmetric
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=3000 %combinedmetric
// clang-format on

// UNSUPPORTED: GNU

#include "combined-metric.h"
#include "delay.h"
#include <omp.h>
#include <stdio.h>
#include <sys/time.h>

#define NCHAINS_FACTOR 1.5       // nchains/nthreads
#define CHAINLENGTH 500          // Number of tasks in a chain
#define DEFAULT_DELAY_TIME 10000 // Default delaytime in microseconds

#undef LB_FAK
#undef UB_FAK
#define LB_FAK 0.8
#define UB_FAK 1.02

int main() {

  /* Initialization */
  double delaylength = DEFAULT_DELAY_TIME;
  int nthreads = omp_get_max_threads();
  int chainlength = CHAINLENGTH;
  double factor = NCHAINS_FACTOR;
  int nchains = (int)(nthreads * factor);
  factor = nchains / (double)nthreads;
  int loadbal = factor / ceil(factor) * 1000;
  metrics m = {loadbal, 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  double tancp = ceil(factor) * (delaylength / 1000000) * chainlength;
  double tacp = (delaylength / 1000000) * chainlength;
  printf("Expected %d < TACP < %d\n", (int)(tacp * LB_FAK),
         (int)ceil(tacp * UB_FAK));
  printf("Expected %d < TACP < %d\n", (int)(tacp * LB_FAK),
         (int)ceil(tacp * UB_FAK));
  printf("Expected %d < TANCP < %d\n", (int)(tancp * LB_FAK),
         (int)ceil(tancp * UB_FAK));
  printf("Expected %d < TANCP < %d\n", (int)(tancp * LB_FAK),
         (int)ceil(tancp * UB_FAK));
  omp_control_tool(omp_control_tool_start, 0, NULL);
  double *dep_arr = malloc(sizeof(double) * nchains);

#pragma omp parallel
  {
#pragma omp single
    {
      int i, j;
      struct timeval task_creation_start, task_creation_end;
      for (i = 0; i < nchains; i++) {
        for (j = 0; j < chainlength; j++) {
#pragma omp task depend(inout : dep_arr[i])
          { delay_sleep(delaylength); }
        }
      }
    }
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}