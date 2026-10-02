// ALLOW_RETRIES: 1
// RUN: env OMP_NUM_THREADS=2 %load_cpt %t
// RUN: env OMP_NUM_THREADS=4 %load_cpt %t
// RUN: env OMP_NUM_THREADS=8 %load_cpt %t

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

#include "expected-metrics.h"

int main(int argc, char **argv) {
  int nt = omp_get_max_threads();
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp single
    {
      /* #pragma omp taskloop if(0)
      for (int i = 0; i < nt; i++) {
            usleep(2*WORK);
      } */
      for (int i = 0; i < nt; i++) {
#pragma omp task if (0)
        { usleep(2 * WORK); }
      }
    } // single
  } // parallel
  omp_control_tool(omp_control_tool_end, 0, NULL);
}
