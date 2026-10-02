// ALLOW_RETRIES: 1
// RUN: env OMP_NUM_THREADS=2 %load_cpt %t
// RUN: env OMP_NUM_THREADS=4 %load_cpt %t
// RUN: env OMP_NUM_THREADS=8 %load_cpt %t

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#include "expected-metrics.h"

int fib(int n) {
  int i, j;
  if (n < 2)
    return n;
  else {
#pragma omp task shared(i) if (n >= 4) mergeable
    i = fib(n - 1);
#pragma omp task shared(j) if (n >= 4) mergeable
    j = fib(n - 2);
#pragma omp taskwait
    return i + j;
  }
}

int main(int argc, char **argv) {
  int res, nt = omp_get_max_threads();
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
  {
#pragma omp single
    { res = fib(10); } // single
  } // parallel
  omp_control_tool(omp_control_tool_end, 0, NULL);
  printf("res = %i\n", res);
}
