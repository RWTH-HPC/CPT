#include <stdio.h>

typedef struct metrics {
  int OLB;
  int OSE;
  int OTE;
  int MLB;
  int MSE;
  int MTE;
} metrics;

#ifndef WORK
#define WORK 100000
#endif

#define printC(m)                                                              \
  printf("Expected %i < " #m " < %i\n", (int)(m * LB_FAK),                     \
         (int)(m * UB_FAK + 0.5));                                             \
  printf("Expected %i < " #m " < %i\n", (int)(m * LB_FAK),                     \
         (int)(m * UB_FAK + 0.5))
#define printM(M)                                                              \
  printf("Expected %i < " #M " < %i\n", (int)(m.M * LB_FAK),                   \
         (int)(m.M * UB_FAK + 0.5));                                           \
  printf("Expected %i < " #M " < %i\n", (int)(m.M * LB_FAK),                   \
         (int)(m.M * UB_FAK + 0.5))

// The printMetrics function prints expected value ranges according to the
// values expected for a test. Each metric is printed twice to allow the
// matching in the CHECK lines below.

static void printMetrics(struct metrics m) {
  int OPE = m.OLB * m.OSE * m.OTE / 1000 / 1000;
  int MPE = m.MLB * m.MSE * m.MTE / 1000 / 1000;
  int LB = m.MLB * m.OLB / 1000;
  int SE = m.MSE * m.OSE / 1000;
  int TE = m.MTE * m.OTE / 1000;
  int PE = OPE * MPE / 1000;
  printC(PE);
  printC(LB * SE);
  printC(SE);
  printC(TE);
  printC(MPE);
  printM(MLB);
  printM(MSE);
  printM(MTE);
  printC(OPE);
  printM(OLB);
  printM(OSE);
  printM(OTE);
}

void printmetrics_(int *olb, int *ose, int *ote, int *mlb, int *mse, int *mte) {
  metrics m = {*olb, *ose, *ote, *mlb, *mse, *mte};
  printMetrics(m);
}

// These check lines assure that the metric values calculated and reported
// by the tool are within error boundaries around the expected values.
// With FileCheck we can only match for exact numbers. So, the approach is:
// 1. get the expected range of values
// 2. get the values reported by the tool
// 3. check that the reported value is larger/smaller than the expected value
// We need the same line to match for step 1 and 3. Each line can only be
// consumed once, therefore each metric is printed twice.

// The first two lines make sure to consume CHECK2 and CHECK4 markers

// clang-format off
// CHECK2-NOT: qwertz
// CHECK4-NOT: qwertz
// CHECK-DAG: Expected [[#%d,PELB:]] < PE < [[#%d,PEUB:]]
// CHECK-DAG: Expected [[#%d,LBLB:]] < LB * SE < [[#%d,LBUB:]]
// CHECK-DAG: Expected [[#%d,SELB:]] < SE < [[#%d,SEUB:]]
// CHECK-DAG: Expected [[#%d,TELB:]] < TE < [[#%d,TEUB:]]
// CHECK-DAG: Expected [[#%d,MPELB:]] < MPE < [[#%d,MPEUB:]]
// CHECK-DAG: Expected [[#%d,MLBLB:]] < MLB < [[#%d,MLBUB:]]
// CHECK-DAG: Expected [[#%d,MSELB:]] < MSE < [[#%d,MSEUB:]]
// CHECK-DAG: Expected [[#%d,MTELB:]] < MTE < [[#%d,MTEUB:]]
// CHECK-DAG: Expected [[#%d,OPELB:]] < OPE < [[#%d,OPEUB:]]
// CHECK-DAG: Expected [[#%d,OLBLB:]] < OLB < [[#%d,OLBUB:]]
// CHECK-DAG: Expected [[#%d,OSELB:]] < OSE < [[#%d,OSEUB:]]
// CHECK-DAG: Expected [[#%d,OTELB:]] < OTE < [[#%d,OTEUB:]]

// CHECK-TASK-DAG: Expected [[#%d,TACPLB:]] < TACP < [[#%d,TACPUB:]]

// CHECK-DAG: Parallel Efficiency:                [[#%d,PE:]]
// CHECK-DAG:   Load Balance:                     [[#%d,LB:]]
// CHECK-DAG:   Communication Efficiency:         [[#%d,CE:]]
// CHECK-DAG:     Serialisation Efficiency:       [[#%d,SE:]]
// CHECK-DAG:     Transfer Efficiency:            [[#%d,TE:]]
// SKIP-CHECK:   MPI Parallel Efficiency:          [[#%d,MPE:]]
// SKIP-CHECK:     MPI Load Balance:               [[#%d,MLB:]]
// SKIP-CHECK:     MPI Communication Efficiency:   [[#%d,MCE:]]
// SKIP-CHECK:       MPI Serialisation Efficiency: [[#%d,MSE:]]
// SKIP-CHECK:       MPI Transfer Efficiency:      [[#%d,MTE:]]
// SKIP-CHECK:   OMP Parallel Efficiency:          [[#%d,OPE:]]
// SKIP-CHECK:     OMP Load Balance:               [[#%d,OLB:]]
// SKIP-CHECK:     OMP Communication Efficiency:   [[#%d,OCE:]]
// SKIP-CHECK:       OMP Serialisation Efficiency: [[#%d,OSE:]]
// SKIP-CHECK:       OMP Transfer Efficiency:      [[#%d,OTE:]]

// CHECK-TASK-DAG: taskCreate:                [[#%d,TCR:==ETCR]]
// CHECK-TASK-DAG: Task-centric: [[#%d,TACP:]]{{.*}}

// CHECK-DAG: Expected [[#%d,min(PELB,PE)]] < PE < [[#%d,max(PEUB,PE)]]
// CHECK-DAG: Expected [[#%d,min(LBLB, mul(LB, SE))]] < LB * SE < [[#%d,max(LBUB, mul(LB, SE))]]
// SKIP-CHECK: Expected [[#%d,min(SELB,SE)]] < SE < [[#%d,max(SEUB,SE)]]
// CHECK-DAG: Expected [[#%d,min(TELB,TE)]] < TE < [[#%d,max(TEUB,TE)]]
// SKIP-CHECK: Expected [[#%d,min(MPELB,MPE)]] < MPE < [[#%d,max(MPEUB,MPE)]]
// SKIP-CHECK: Expected [[#%d,min(MLBLB,MLB)]] < MLB < [[#%d,max(MLBUB,MLB)]]
// SKIP-CHECK: Expected [[#%d,min(MSELB,MSE)]] < MSE < [[#%d,max(MSEUB,MSE)]]
// SKIP-CHECK: Expected [[#%d,min(MTELB,MTE)]] < MTE < [[#%d,max(MTEUB,MTE)]]
// SKIP-CHECK: Expected [[#%d,min(OPELB,OPE)]] < OPE < [[#%d,max(OPEUB,OPE)]]
// SKIP-CHECK: Expected [[#%d,min(OLBLB,OLB)]] < OLB < [[#%d,max(OLBUB,OLB)]]
// SKIP-CHECK: Expected [[#%d,min(OSELB,OSE)]] < OSE < [[#%d,max(OSEUB,OSE)]]
// SKIP-CHECK: Expected [[#%d,min(OTELB,OTE)]] < OTE < [[#%d,max(OTEUB,OTE)]]

// CHECK-TASK-DAG: Expected [[#%d,min(TACPLB,TACP)]] < TACP < [[#%d,max(TACPUB,TACP)]]
// clang-format on
