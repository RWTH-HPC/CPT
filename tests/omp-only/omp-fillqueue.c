// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck %s
// clang-format on

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#include "ompt-signal.h"

int main() {
  int nt = omp_get_max_threads();
  int a = 0;
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp master
    {
      for (int i = 0; i < 300; i++) {
#pragma omp task shared(a)
        { OMPT_SIGNAL(a); }
      }
#pragma omp taskwait
    }
    OMPT_WAIT(a, 300);
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
  printf("Threads: %d\n", nt);
  return 0;
}
