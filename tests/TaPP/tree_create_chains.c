// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c_delay
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=1100 %combinedmetric
// RUN: env OMP_NUM_THREADS=4 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=2000 %combinedmetric
// clang-format on

// UNSUPPORTED: GNU

#include "combined-metric.c"
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

void recursive_task_branch(int depth, double delaylength) {
  delay_sleep(delaylength);
  if (depth > 0) {
#pragma omp task
    { recursive_task_branch(depth - 1, delaylength); }
  }
}

void recursive_task(int depth, double delaylength, int nbranches) {
  delay_sleep(delaylength);
  if (depth > 0) {
#pragma omp task
    { recursive_task(depth - 1, delaylength, nbranches); }
  } else if (depth == 0) {
    int i;
    for (i = 0; i < nbranches; i++) {
#pragma omp task
      { recursive_task_branch(BRANCH_CHAINLENGTH - 1, delaylength); }
    }
  }
}

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
#pragma omp parallel
  {
#pragma omp single
    {
      if (INIT_CHAINLENGTH > 0) {
#pragma omp task
        { recursive_task(INIT_CHAINLENGTH - 1, delaylength, nbranches); }
      }
    }
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
}