// clang-format off
// ALLOW_RETRIES: 1
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=2 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck %s

// UNSUPPORTED: GNU,icc-2021.6,icc-2021.7,icc-2021.9,intelllvm-2022.1,intelllvm-2022.2,intelllvm-2023.1
// clang-format on

// Note: adapted from task_late_fulfill.c test from LLVM

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

int main() {
  int nt = omp_get_max_threads();
  omp_control_tool(omp_control_tool_start, 0, NULL);
#pragma omp parallel
#pragma omp master
  {
    omp_event_handle_t event;
    omp_event_handle_t *f_event;
#pragma omp task detach(event) depend(out : f_event) shared(f_event) if (0)
    {
      printf("task 1\n");
      f_event = &event;
    }
#pragma omp task depend(in : f_event)
    { printf("task 2\n"); }
    printf("calling omp_fulfill_event\n");
    omp_fulfill_event(*f_event);
#pragma omp taskwait
  }
  omp_control_tool(omp_control_tool_end, 0, NULL);
  printf("Threads: %d\n", nt);
  return 0;
}
