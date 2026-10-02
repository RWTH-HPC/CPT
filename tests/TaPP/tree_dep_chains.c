// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c_delay
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=1100 %combinedmetric
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=2000 %combinedmetric
// clang-format on

// UNSUPPORTED: GNU

#include "combined-metric.h"
#include "delay.h"
#include <omp.h>
#include <stdio.h>
#include <sys/time.h>

#define NBRANCHES_RATIO 1.5 // Nbranches/nthreads
#define INIT_CHAINLENGTH 200
#define BRANCH_CHAINLENGTH 300   // Number of tasks in a branch
#define DEFAULT_DELAY_TIME 10000 // Default delaytime in microseconds

#undef LB_FAK
#undef UB_FAK
#define LB_FAK 0.5
#define UB_FAK 1.02

int main() {

  /* Initialization */
  double delaylength = DEFAULT_DELAY_TIME;

  int nthreads = omp_get_max_threads();
  double factor = NBRANCHES_RATIO;
  int nbranches = (int)(nthreads * factor);
  factor = nbranches / (double)nthreads;
  double avg_init = INIT_CHAINLENGTH / (double)nthreads;
  double avg_branch = factor * BRANCH_CHAINLENGTH;
  double tacp =
      (BRANCH_CHAINLENGTH + INIT_CHAINLENGTH) * (delaylength / 1000000);
  printf("Expected %d < TACP < %d\n", (int)(tacp * LB_FAK),
         (int)ceil(tacp * UB_FAK));
  printf("Expected %d < TACP < %d\n", (int)(tacp * LB_FAK),
         (int)ceil(tacp * UB_FAK));
  int lb = (avg_init + avg_branch) /
           (ceil(factor) * BRANCH_CHAINLENGTH + INIT_CHAINLENGTH) * 1000;
  metrics m = {lb, 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);

  double *dep_arr = malloc(sizeof(double) * nbranches);
  double x;

#pragma omp parallel
  {
#pragma omp single
    {
      int i, j;
      for (i = 0; i < INIT_CHAINLENGTH; i++) {
#pragma omp task depend(inout : x)
        { delay_sleep(delaylength); }
      }

      for (i = 0; i < nbranches; i++) {
#pragma omp task depend(in : x) depend(out : dep_arr[i])
        { delay_sleep(delaylength); }
        for (j = 0; j < BRANCH_CHAINLENGTH - 1; j++) {
#pragma omp task depend(inout : dep_arr[i])
          { delay_sleep(delaylength); }
        }
      }
    }
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}