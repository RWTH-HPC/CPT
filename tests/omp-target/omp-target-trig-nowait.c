#include <cuda.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <unistd.h>

// REQUIRES: ompt-target
// RUN: true

#define N 100000000
#define M 100
#define L 100
#define O 100

int main(int argc, char **argv) {
  float *A = (float *)malloc(N * sizeof(float));
  float *B = (float *)malloc(N * sizeof(float));
  float *C = (float *)malloc(N * sizeof(float));

#pragma omp parallel for
  for (long i = 0; i < N; i++) {
    A[i] = 1.;
    B[i] = 2.;
  }

  // omp_control_tool(omp_control_tool_start, 0, NULL);

#pragma omp target enter data map(to : A[0 : N], B[0 : N]) map(alloc : C[0 : N])

#pragma omp target teams distribute parallel for nowait depend(inout : A)
  for (long i = 0; i < N; i++) {
    float x = A[i];
    for (int j = 0; j < M; j++) {
      x = sin(x) * cos(x) + tan(x / (x + 1e-6));
    }
    A[i] = x;
  }

#pragma omp target teams distribute parallel for nowait depend(inout : B)
  for (long i = 0; i < N; i++) {
    float y = B[i];
    for (int j = 0; j < L; j++) {
      y = sin(y) / (cos(y) + 1e-6);
    }
    B[i] = y;
  }

#pragma omp target teams distribute parallel for nowait depend(in : A, B)      \
    depend(out : C)
  for (long i = 0; i < N; i++) {
    float x = A[i], y = B[i];
    for (int j = 0; j < O; j++) {
      x = sin(x) + log(fabs(y) + 1e-6);
      y = cos(y) + exp(-fabs(x));
    }
    C[i] = x / (y + 1e-6) + tan(x * y);
  }

#pragma omp taskwait

#pragma omp target exit data map(from : A[0 : N], B[0 : N], C[0 : N])

  // omp_control_tool(omp_control_tool_end, 0, NULL);

  printf("Res: %f, %f, %f\n", A[0], B[0], C[0]);

  free(A);
  free(B);
  free(C);
}
