// ALLOW_RETRIES: 1
// RUN: env OMP_NUM_THREADS=2 %load_cpt %t
// RUN: env OMP_NUM_THREADS=4 %load_cpt %t
// RUN: env OMP_NUM_THREADS=8 %load_cpt %t

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int sum, nt = omp_get_max_threads();
  int *parts = malloc(sizeof(int) * nt);
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp single nowait
    {
      for (int i = 0; i < nt; i++) {
#pragma omp task
        {
          parts[i] = i;
          usleep(2 * WORK);
        } // inner tasks
      }
    } // single
#pragma omp barrier
#pragma omp taskwait
#pragma omp single
    {
      for (int i = 0; i < nt; i++) {
        sum += parts[i];
      }
    }
  } // parallel
  omp_control_tool(omp_control_tool_end, 0, NULL);
  free(parts);
  printf("sum = %i\n", sum);
}
