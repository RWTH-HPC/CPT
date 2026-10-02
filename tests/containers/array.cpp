// clang-format off
// RUN: %t
// clang-format on

#include "containers.h"

int main() {
  Array<int, 5> A = {{1, 2, 3, 4, 5}}, B{};
  for (auto b : B)
    CHECK_EQ(b, 0);
  for (int i = 0; i < A.size(); i++)
    CHECK_EQ(A[i], i + 1);
  B += A;
  for (int i = 0; i < B.size(); i++)
    CHECK_EQ(B[i], i + 1);
  B -= A;
  for (auto b : B)
    CHECK_EQ(b, 0);
}