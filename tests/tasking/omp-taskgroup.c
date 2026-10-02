// ALLOW_RETRIES: 5
// clang-format off
// RUN: %compile_c
// RUN: env OMP_NUM_THREADS=3 %load_cpt %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=7 %combinedmetric
// RUN: env OMP_NUM_THREADS=6 %load_cpt_omp %cpt_options_dump_stopped \
// RUN: %t | %FileCheck --check-prefixes=CHECK,CHECK-TASK -D#ETCR=10 %combinedmetric

// UNSUPPORTED: GNU

// The 50% PE can result in 50% LB or 50% SE, depending on the actual schedule of the tasks
// SE case:
//  Thr1  |  Thr2  |  Thr3
//   -    |  Task1 |  Task2
//    end taskgroup
//  Task3 |   -    |   - 
// LB case:
//  Thr1  |  Thr2  |  Thr3
//   -    |  Task1 |  Task2
//    end taskgroup
//        |  Task3 |   -
// clang-format on

#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#undef LB_FAK
#undef UB_FAK
#define LB_FAK 0.5
#define UB_FAK 1.02

#include "combined-metric.h"

int main(int argc, char **argv) {
  int nt = omp_get_max_threads();
  if (nt < 3 || nt % 3 != 0) {
    printf("Please select a thread count divisible by 3!\n");
    return 1;
  }

  metrics m = {500, 1000, 1000, 1000, 1000, 1000};
  printf("Expected %d < TACP < %d\n", (int)(0.2 * LB_FAK),
         (int)ceil(0.2 * UB_FAK));
  printf("Expected %d < TACP < %d\n", (int)(0.2 * LB_FAK),
         (int)ceil(0.2 * UB_FAK));
  printf("Expected %d < TANCP < %d\n", (int)(0.2 * LB_FAK),
         (int)ceil(0.2 * UB_FAK));
  printf("Expected %d < TANCP < %d\n", (int)(0.2 * LB_FAK),
         (int)ceil(0.2 * UB_FAK));
  printMetrics(m);

  omp_control_tool(omp_control_tool_start, 0, NULL);

#pragma omp parallel
  {
#pragma omp single
    {
#pragma omp taskgroup
      {
#pragma omp task
        {
#pragma omp task
          {
            for (int i = 0; i < nt / 3; i++) {
#pragma omp task
              { usleep(WORK); }
            }
          }
#pragma omp task
          {
            for (int i = 0; i < nt / 3; i++) {
#pragma omp task
              { usleep(WORK); }
            }
          }
#pragma omp taskwait
        }
      }
#pragma omp task
      {
        for (int i = 0; i < nt / 3; i++) {
#pragma omp task
          { usleep(WORK); }
        }
      }
    } // single
  } // parallel

  omp_control_tool(omp_control_tool_end, 0, NULL);
}
