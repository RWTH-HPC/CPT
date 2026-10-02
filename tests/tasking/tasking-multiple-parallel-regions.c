// RUN: true

// UNSUPPORTED: GNU

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#define delay(t) usleep(t);

int main(int argc, char **argv) {
  int condition = 0;
  usleep(100000);
#pragma omp parallel
  {
    usleep(100000);
#pragma omp master
    {
      for (int i = 0; i < 4; i++) {
#pragma omp task shared(condition)
        {
          int tid = omp_get_thread_num();
          usleep(100000);
        }
      }
    }
#pragma omp barrier
  }
  usleep(150000);
#pragma omp parallel
  {
    usleep(100000);
#pragma omp master
    {
      for (int i = 0; i < 4; i++) {
#pragma omp task shared(condition)
        {
          int tid = omp_get_thread_num();
          usleep(100000);
        }
      }
    }
#pragma omp barrier
  }
}
