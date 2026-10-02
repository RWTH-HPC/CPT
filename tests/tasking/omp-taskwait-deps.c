// ALLOW_RETRIES: 3
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %combinedmetric
// RUN: env OMP_NUM_THREADS=4 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %combinedmetric
// RUN: env OMP_NUM_THREADS=8 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK2,CHECK %combinedmetric

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#undef LB_FAK
#undef UB_FAK
#define LB_FAK 0.8
#define UB_FAK 1.02

#include "combined-metric.h"

int main(int argc, char **argv) {
  int sum, nt = omp_get_max_threads();
  int *parts = malloc(sizeof(int) * nt);
  metrics m = {(int)(1000 / nt), 1000, 1000, 1000, 1000, 1000};
  printMetrics(m);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp single
    {
      for (int i = 0; i < nt; i++) {
#pragma omp task depend(out : sum)
        {
          parts[i] = i;
          usleep(2 * WORK);
        } // antecedent tasks
      }
#pragma omp task depend(in : sum)
      {
        for (int i = 0; i < nt; i++) {
          sum += parts[i];
        }
      } // dependent task
#pragma omp taskwait
    } // single
  } // parallel
  omp_control_tool(omp_control_tool_end, 0, NULL);
  free(parts);
  printf("sum = %i\n", sum);
}