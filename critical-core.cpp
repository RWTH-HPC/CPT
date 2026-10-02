/*
 * critPathAnalysis.cpp -- Critical path analysis runtime library, build for
 * hybrid OpenMp and MPI applications
 */

//===----------------------------------------------------------------------===//
//
// Based on the ompt-tsan.cpp of the LLVM Project
// version as of 06/24/2021
// parent 82e4e50 commit 08d8f1a958bd8be681e3e1f346be80818a83a556
// See https://llvm.org/LICENSE.txt for details.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "containers.h"
#include "criticalPath.h"
#include "errorhandler.h"
#include "parse_flags.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <time.h>
#include <unistd.h>
#ifdef USE_MPI
#include <mpi.h>
#endif

extern "C" void __cxa_pure_virtual() {
  DCHECK(false && "Pure virtual function must not be called");
  abort();
}

#ifdef USE_ERRHANDLER
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static void set_signalhandlers(sighandler_t handler) {
  signal(SIGSEGV, handler);
  signal(SIGINT, handler);
  signal(SIGHUP, handler);
  signal(SIGABRT, handler);
  signal(SIGTERM, handler);
  signal(SIGUSR2, handler);
  signal(SIGQUIT, handler);
  signal(SIGALRM, handler);
}

void disable_signalhandlers() { set_signalhandlers(SIG_DFL); }

void mySignalHandler(int signum) {
  disable_signalhandlers();
  printf("pid %i caught signal nr %i\n", getpid(), signum);
  if (signum == SIGINT || signum == SIGKILL) {
    print_stack();
    _exit(signum + 128);
  }
  if (signum == SIGTERM || signum == SIGUSR2) {
    print_stack();
    fflush(stdout);
    sleep(1);
    _exit(signum + 128);
  }
  print_stack();

  printf("Waiting up to %i seconds to attach with a debugger.\n", 20);
  sleep(20);
  _exit(1);
}

void init_signalhandlers() {
  if (!analysis_flags->stacktrace)
    return;
  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Setting signal handlers\n");
  set_signalhandlers(mySignalHandler);
}

#endif

double localTimeOffset{0};
long long startTimeOffset{0};
double getTime() {
#ifdef NOTIME
  static thread_local double time = 0;
  time += .1;
  return time;
#else
  struct timespec curr;
  clock_gettime(CLOCK_REALTIME, &curr);
  return curr.tv_sec - startTimeOffset + curr.tv_nsec * 1e-9 - localTimeOffset;
#endif
}

int myProcId = 0;
bool useMpi = false;

ompt_finalize_tool_t critical_ompt_finalize_tool{nullptr};

uint64_t my_next_id() {
  static uint64_t ID = 0;
  uint64_t ret = __sync_fetch_and_add(&ID, 1);
  return ret;
}

// Specializations that determine whether a metric class needs locking in
// SyncClock
template <> UniqLock<DependentMetric>::UniqLock(std::mutex &m) : u(m) {}
template <> UniqLock<TimeMetric>::UniqLock(std::mutex &m) : u() {}

double totalProgrammTime = 0;
double startProgrammTime = getTime(), endProgrammTime = startProgrammTime;
double crit_path_useful_time = 0;

Vector<THREAD_CLOCK *> *thread_clocks = nullptr;
Vector<omptCounts<double> *> *thread_counts = nullptr;
thread_local THREAD_CLOCK *thread_local_clock = nullptr;

const char *debug_clock_state_string[] = {
    "STATE_UNINIT", "STATE_INIT", "STATE_NONE", "STATE_USEFUL",
    "STATE_MPI",    "STATE_OMP",  "STATE_GPU",  "STATE_LAST"};

#ifdef DEBUG_CLOCKS
DebugClocksRAII::DebugClocksRAII(THREAD_CLOCK *_tc, const char *_loc,
                                 const char *_func)
    : tc(_tc), loc(_loc), func(_func) {
  std::lock_guard<std::mutex> guard(debugClockMutex);
  fprintf(analysis_flags->output, "\n");
  tc->Print(func, loc, " (before)");
  tc->printStateStack(loc, " (before)");
  fflush(analysis_flags->output);
}
DebugClocksRAII::~DebugClocksRAII() {
  std::lock_guard<std::mutex> guard(debugClockMutex);
  tc->Print(func, loc, " (after)");
  tc->printStateStack(loc, " (after)");
  fprintf(analysis_flags->output, "\n");
  fflush(analysis_flags->output);
}
#endif

void enterOpenMP(const char *loc) {
  thread_local_clock->enterState(STATE_OMP, loc);
}
void exitOpenMP(const char *loc) {
  thread_local_clock->exitState(STATE_OMP, STATE_USEFUL, loc);
}

void startMeasurement(double time) {
  auto initialStart = startProgrammTime;
  startProgrammTime = time;
  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Initialize after %lf s\n",
            startProgrammTime - initialStart);
}

void stopMeasurement(double time) { endProgrammTime = time; }

template <>
double atomic_add<double>(std::atomic<double> &operand, double value_to_add) {
  double old = operand.load(std::memory_order_consume);
  double desired = old + value_to_add;
  while (!operand.compare_exchange_weak(old, desired, std::memory_order_release,
                                        std::memory_order_consume))
    desired = old + value_to_add;

  return desired;
}

// different types of averages and maximum. Here: over all threads, over all
// processes
#define NUM_SHARED_METRICS 2

void calculate_average_and_max(
    Vector<THREAD_CLOCK *> &clocks, double totalRuntimeReal,
    double (&avgComputation)[NUM_SHARED_METRICS],
    double (&maxComputation)[NUM_SHARED_METRICS],
    double (&avgIntDepVals)[NUM_SHARED_METRICS][NUM_UC_INT64],
    int (&maxIntDepVals)[NUM_SHARED_METRICS][NUM_UC_INT64],
    double (&avgDoubleDepVals)[NUM_SHARED_METRICS][NUM_UC_DOUBLE],
    double (&maxDoubleDepVals)[NUM_SHARED_METRICS][NUM_UC_DOUBLE],
    int &number_of_procs, int &total_threads, MPI_COUNTS &total_counts) {
  number_of_procs = 1;
  total_threads = 0;
  int num_threads = 0;

  total_threads = num_threads = clocks.Size();
  bool initialConsidered = false;

  double uc_avg[NUM_SHARED_METRICS] = {0};
  double uc_max[NUM_SHARED_METRICS] = {0};
  // for dep metrics
#if NUM_UC_INT64 > 0
  int uc_int_dep_avg[NUM_SHARED_METRICS][NUM_UC_INT64] = {{0}};
  int uc_int_dep_max[NUM_SHARED_METRICS][NUM_UC_INT64] = {{0}};
#endif
#if NUM_UC_DOUBLE > 0
  double uc_dbl_dep_avg[NUM_SHARED_METRICS][NUM_UC_DOUBLE] = {{0}};
  double uc_dbl_dep_max[NUM_SHARED_METRICS][NUM_UC_DOUBLE] = {{0}};
#endif

  MPI_COUNTS proc_counts;
  for (int i = 0; i < num_threads; i++) {
    bool isInitial = ((*thread_clocks)[i]->thread_type == ompt_thread_initial);
    if (initialConsidered && isInitial)
      continue;
    if (isInitial)
      initialConsidered = true;
    auto *tclock = (clocks)[i];
    proc_counts.add(*tclock);
    double curr_uc = tclock->clocks[CLOCK_USEFUL].thread.getTime();
#if NUM_UC_INT64 > 0
    Array<uint64_t, NUM_UC_INT64> *curr_int_uc =
        tclock->clocks[CLOCK_USEFUL].thread.getIntDeps();
#endif
#if NUM_UC_DOUBLE > 0
    Array<double, NUM_UC_DOUBLE> *curr_dbl_uc =
        tclock->clocks[CLOCK_USEFUL].thread.getDoubleDeps();
#endif

    if (curr_uc > uc_max[0]) {
      uc_max[0] = curr_uc;
#if NUM_UC_INT64 > 0
      for (int j = 0; j < NUM_UC_INT64; j++) {
        uc_int_dep_max[0][j] = (*curr_int_uc)[j];
      }
#endif
#if NUM_UC_DOUBLE > 0
      for (int j = 0; j < NUM_UC_DOUBLE; j++) {
        uc_dbl_dep_max[0][j] = (*curr_dbl_uc)[j];
      }
#endif
    }
    uc_avg[0] += curr_uc;
#if NUM_UC_INT64 > 0
    for (int j = 0; j < NUM_UC_INT64; j++) {
      uc_int_dep_avg[0][j] += (*curr_int_uc)[j];
    }
#endif
#if NUM_UC_DOUBLE > 0
    for (int j = 0; j < NUM_UC_DOUBLE; j++) {
      uc_dbl_dep_avg[0][j] += (*curr_dbl_uc)[j];
    }
#endif
  }

  uc_max[1] = uc_avg[1] = (clocks)[0]->clocks[CLOCK_USEFUL].proc.getTime();
#if NUM_UC_INT64 > 0
  for (int j = 0; j < NUM_UC_INT64; j++) {
    uc_int_dep_max[1][j] = uc_int_dep_avg[1][j] =
        (*((clocks)[0]->clocks[CLOCK_USEFUL].proc.getIntDeps()))[j];
  }
#endif
#if NUM_UC_DOUBLE > 0
  for (int j = 0; j < NUM_UC_DOUBLE; j++) {
    uc_dbl_dep_max[1][j] = uc_dbl_dep_avg[1][j] =
        (*((clocks)[0]->clocks[CLOCK_USEFUL].proc.getDoubleDeps()))[j];
  }
#endif

#ifdef USE_MPI
  if (useMpi) {
    // aggregate all max and avg computations on the master thread
    PMPI_Reduce(&uc_max, &maxComputation, NUM_SHARED_METRICS, MPI_DOUBLE,
                MPI_MAX, 0, MPI_COMM_WORLD);
    PMPI_Reduce(&uc_avg, &avgComputation, NUM_SHARED_METRICS, MPI_DOUBLE,
                MPI_SUM, 0, MPI_COMM_WORLD);
#if NUM_UC_INT64 > 0
    PMPI_Reduce(&uc_int_dep_max, &maxIntDepVals,
                NUM_SHARED_METRICS * NUM_UC_INT64, MPI_INT, MPI_MAX, 0,
                MPI_COMM_WORLD);
    PMPI_Reduce(&uc_int_dep_avg, &avgIntDepVals,
                NUM_SHARED_METRICS * NUM_UC_INT64, MPI_INT, MPI_SUM, 0,
                MPI_COMM_WORLD);
#endif
#if NUM_UC_DOUBLE > 0
    PMPI_Reduce(&uc_dbl_dep_max, &maxDoubleDepVals,
                NUM_SHARED_METRICS * NUM_UC_DOUBLE, MPI_DOUBLE, MPI_MAX, 0,
                MPI_COMM_WORLD);
    PMPI_Reduce(&uc_dbl_dep_avg, &avgDoubleDepVals,
                NUM_SHARED_METRICS * NUM_UC_DOUBLE, MPI_DOUBLE, MPI_SUM, 0,
                MPI_COMM_WORLD);
#endif
    PMPI_Reduce(&num_threads, &total_threads, 1, MPI_INT, MPI_SUM, 0,
                MPI_COMM_WORLD);
    PMPI_Reduce(&proc_counts, &total_counts,
                sizeof(proc_counts) / sizeof(uint64_t), MPI_UINT64_T, MPI_SUM,
                0, MPI_COMM_WORLD);
    double localRuntimeReal = totalRuntimeReal;
    PMPI_Reduce(&localRuntimeReal, &totalRuntimeReal, 1, MPI_DOUBLE, MPI_MAX, 0,
                MPI_COMM_WORLD);
    PMPI_Comm_size(MPI_COMM_WORLD, &number_of_procs);
    avgComputation[1] = avgComputation[1] / number_of_procs;
    avgComputation[0] = avgComputation[0] / total_threads;
#if NUM_UC_INT64 > 0
    for (int j = 0; j < NUM_UC_INT64; j++) {
      avgIntDepVals[1][j] = avgIntDepVals[1][j] / (double)number_of_procs;
      avgIntDepVals[0][j] = avgIntDepVals[0][j] / (double)total_threads;
    }
#endif
#if NUM_UC_DOUBLE > 0
    for (int j = 0; j < NUM_UC_DOUBLE; j++) {
      avgDoubleDepVals[1][j] = avgDoubleDepVals[1][j] / (double)number_of_procs;
      avgDoubleDepVals[0][j] = avgDoubleDepVals[0][j] / (double)total_threads;
    }
#endif
  } else
#endif
  {
    for (int i = 0; i < NUM_SHARED_METRICS; i++) {
      maxComputation[i] = uc_max[i];
      avgComputation[i] = uc_avg[i];
#if NUM_UC_INT64 > 0
      for (int j = 0; j < NUM_UC_INT64; j++) {
        maxIntDepVals[i][j] = uc_int_dep_max[i][j];
        avgIntDepVals[i][j] = uc_int_dep_avg[i][j];
      }
#endif
#if NUM_UC_DOUBLE > 0
      for (int j = 0; j < NUM_UC_DOUBLE; j++) {
        maxDoubleDepVals[i][j] = uc_dbl_dep_max[i][j];
        avgDoubleDepVals[i][j] = uc_dbl_dep_avg[i][j];
      }
#endif
    }
    avgComputation[0] /= num_threads;
#if NUM_UC_INT64 > 0
    for (int j = 0; j < NUM_UC_INT64; j++) {
      avgIntDepVals[0][j] /= (double)num_threads;
    }
#endif
#if NUM_UC_DOUBLE > 0
    for (int j = 0; j < NUM_UC_DOUBLE; j++) {
      avgDoubleDepVals[0][j] /= num_threads;
    }
#endif
  }
}

void print_POP_metrics(const double (&avgComputation)[NUM_SHARED_METRICS],
                       const double (&maxComputation)[NUM_SHARED_METRICS],
                       double totalRuntimeReal,
                       double totalRuntimeRealCorrected,
                       double totalRuntimeIdeal, FILE *of) {
  double CommE = maxComputation[0] / totalRuntimeReal;
  double TE = totalRuntimeIdeal / totalRuntimeReal;
  double SerE = maxComputation[0] / totalRuntimeIdeal;
  double LB = avgComputation[0] / maxComputation[0];
  double PE = LB * CommE;

  // corrected values
  double CommE_corr = maxComputation[0] / totalRuntimeRealCorrected;
  double TE_corr = totalRuntimeIdeal / totalRuntimeRealCorrected;
  double PE_corr = LB * CommE_corr;

  if (analysis_flags->verbose) {
    fprintf(
        of,
        "\n[pop] "
        "sere:%6.3lf:te:%6.3lf:comme:%6.3lf:lb:%6.3lf:pe:%6.3lf:crittime:%2."
        "5lf:totaltime:%6.3lf:avgcomputation:%6.3lf:maxcomputation:%6.3lf\n",
        SerE, TE, CommE, LB, PE, totalRuntimeIdeal, totalRuntimeReal,
        avgComputation[0], maxComputation[0]);
  }

  fprintf(of, "\n\n----------------POP metrics----------------\n");
  fprintf(of, "Parallel Efficiency:                %6.3lf\n",
          PE * analysis_flags->metric_factor);
  fprintf(of, "  Load Balance:                     %6.3lf\n",
          LB * analysis_flags->metric_factor);
  fprintf(of, "  Communication Efficiency:         %6.3lf\n",
          CommE * analysis_flags->metric_factor);
  fprintf(of, "    Serialisation Efficiency:       %6.3lf\n",
          SerE * analysis_flags->metric_factor);
  fprintf(of, "    Transfer Efficiency:            %6.3lf\n",
          TE * analysis_flags->metric_factor);
  fprintf(of, "-------------------------------------------\n");
}

void finishMeasurement() {

  if (analysis_flags->running) {
    endProgrammTime = getTime();
    // STATE_INIT to stop clock
    thread_local_clock->setState(endProgrammTime, STATE_INIT, __func__);
    analysis_flags->running = false;
  }

  double totalRuntimeReal = endProgrammTime - startProgrammTime;
  if (analysis_flags->verbose)
    printf("runtime flag: %lf, %lf\n", totalRuntimeReal,
           analysis_flags->runtime);
  if (analysis_flags->runtime > 0)
    totalRuntimeReal = analysis_flags->runtime;

  // tool was never started
  if (startProgrammTime == endProgrammTime) {
    printf("Total Runtime: 0.0\n WARNING: No measurement data found, skipping "
           "tool output. This can happen if the tool was never started.\n");
    return;
  }

#if NUM_UC_INT64 > 0 || NUM_UC_DOUBLE > 0
  // print info regarding dep metrics
  auto depOrga = DepMetricOrganizer::getInstance();
  Vector<IntDepMetricHandler *> *idhs = depOrga->getIntDepMetricHandlers();
  Vector<DoubleDepMetricHandler *> *ddhs =
      depOrga->getDoubleDepMetricHandlers();
  if (analysis_flags->verbose) {
    printf("Num int: %i, num double:%i\n", depOrga->getNumIntValues(),
           depOrga->getNumDoubleValues());
    for (int i = 0; i < idhs->Size(); i++) {
      for (int j = 0; j < (*idhs)[i]->getNumValues(); j++) {
        printf("'%s' ", (*idhs)[i]->getMetricName(j).c_str());
      }
      printf("\n");
    }
    for (int i = 0; i < ddhs->Size(); i++) {
      for (int j = 0; j < (*ddhs)[i]->getNumValues(); j++) {
        printf("'%s' ", (*ddhs)[i]->getMetricName(j).c_str());
      }
      printf("\n");
    }
    thread_local_clock->clocks[CLOCK_USEFUL].critical.printValues();
    if (analysis_flags->tasking)
      thread_local_clock->fiber->clocks[CLOCK_USEFUL].critical.printValues();
  }
#endif

  // postprocess all vector and fiber clocks
  Vector<THREAD_CLOCK *> clocks{};

  if (thread_clocks) {
    bool initialPushed = false;
    for (int i = 0; i < thread_clocks->Size(); i++) {
      bool isInitial =
          ((*thread_clocks)[i]->thread_type == ompt_thread_initial);
      // We are only interested in worker threads and the first initial thread.
      // All other initial threads should be ignored
      if (!initialPushed || !isInitial) {
        // STATE_INIT to stop all clocks
        if ((*thread_clocks)[i]->GetState() != STATE_INIT)
          (*thread_clocks)[i]->setState(endProgrammTime, STATE_INIT, __func__);
        clocks.PushBack((*thread_clocks)[i]);
        if (isInitial)
          initialPushed = true;
      }
    }
    if (analysis_flags->verbose)
      printf("Clocks size: %lu\n", thread_clocks->Size());
  }
  // If we have an MPI only run, thread_clocks is never set, so instead put
  // thread_local_clock in a vector
  else {
    clocks.PushBack(thread_local_clock);
    thread_clocks = new Vector<THREAD_CLOCK *>{};
    thread_clocks->PushBack(thread_local_clock);
  }

  // Store all fiber clocks that exist at the end of the run  -> others are not
  // of interest for us
  Vector<THREAD_CLOCK *> fiber_clocks{};
  if (analysis_flags->tasking) {
    if (analysis_flags->verbose)
      printf("FiberClocks size: %lu\n", thread_clocks->Size());
    for (int i = 0; i < thread_clocks->Size(); i++) {
      DCHECK((*thread_clocks)[i]->fiber);
      fiber_clocks.PushBack((*thread_clocks)[i]->fiber);
    }
  }

  fflush(NULL);

  // Sum up OpenMP thread counts and maxima
  double maxTimeTaskCreate, maxTimeTaskSchedule, maxTimeSync,
      maxTimeDependences, maxTotal, avgTotal = 0.0;
  if (thread_counts) {
    if (analysis_flags->verbose)
      printf(
          "Taskvals on 0: %i, %f, %f, %f, %i, %i, %i, %i\n",
          (*thread_counts)[0]->totalTasks, (*thread_counts)[0]->minTaskTime,
          (*thread_counts)[0]->maxTaskTime, (*thread_counts)[0]->totalTaskTime,
          (*thread_counts)[0]->implTaskBegin, (*thread_counts)[0]->implTaskEnd,
          (*thread_counts)[0]->taskCreate, (*thread_counts)[0]->taskSchedule);
    for (int i = 1; i < thread_counts->Size(); i++) {
      if (analysis_flags->verbose)
        printf(
            "Taskvls on %i: %i, %f, %f, %f, %i, %i, %i, %i\n", i,
            (*thread_counts)[i]->totalTasks, (*thread_counts)[i]->minTaskTime,
            (*thread_counts)[i]->maxTaskTime,
            (*thread_counts)[i]->totalTaskTime,
            (*thread_counts)[i]->implTaskBegin,
            (*thread_counts)[i]->implTaskEnd, (*thread_counts)[i]->taskCreate,
            (*thread_counts)[i]->taskSchedule);
      double totalOverhead = (*thread_counts)[i]->timeTaskCreate +
                             (*thread_counts)[i]->timeTaskSchedule +
                             (*thread_counts)[i]->timeSync +
                             (*thread_counts)[i]->timeDependences;
      if ((*thread_counts)[i]->timeTaskCreate > maxTimeTaskCreate)
        maxTimeTaskCreate = (*thread_counts)[i]->timeTaskCreate;
      if ((*thread_counts)[i]->timeTaskSchedule > maxTimeTaskSchedule)
        maxTimeTaskSchedule = (*thread_counts)[i]->timeTaskSchedule;
      if ((*thread_counts)[i]->timeSync > maxTimeSync)
        maxTimeSync = (*thread_counts)[i]->timeSync;
      if ((*thread_counts)[i]->timeDependences > maxTimeDependences)
        maxTimeDependences = (*thread_counts)[i]->timeDependences;
      if (totalOverhead > maxTotal)
        maxTotal = totalOverhead;
      avgTotal += totalOverhead;
      (*thread_counts)[0]->add(*(*thread_counts)[i]);
    }
    avgTotal /= thread_counts->Size();
  }

  double runtimeCorrectedMaxOverhead1 = totalRuntimeReal - maxTimeTaskCreate -
                                        maxTimeTaskSchedule - maxTimeSync -
                                        maxTimeDependences;
  double runtimeCorrectedMaxOverhead2 = totalRuntimeReal - maxTotal;
  double runtimeCorrectedAvgOverhead = totalRuntimeReal - avgTotal;
  double runtimeNoTool = runtimeCorrectedAvgOverhead;
  if (strcmp("0.0", analysis_flags->no_tool_runtime) != 0) {
    runtimeNoTool = strtod(analysis_flags->no_tool_runtime, nullptr);
  }

  // Calculate the average and maximum over all threads and processes
  double avgComputation[NUM_SHARED_METRICS] = {0};
  double maxComputation[NUM_SHARED_METRICS] = {0};
  double fiberAvgComputation[NUM_SHARED_METRICS] = {0};
  double fiberMaxComputation[NUM_SHARED_METRICS] = {0};

  // dep metrics
  double avgIntDepVals[NUM_SHARED_METRICS][NUM_UC_INT64];
  int maxIntDepVals[NUM_SHARED_METRICS][NUM_UC_INT64];
  double avgDoubleDepVals[NUM_SHARED_METRICS][NUM_UC_DOUBLE];
  double maxDoubleDepVals[NUM_SHARED_METRICS][NUM_UC_DOUBLE];
  double fiberAvgIntDepVals[NUM_SHARED_METRICS][NUM_UC_INT64];
  int fiberMaxIntDepVals[NUM_SHARED_METRICS][NUM_UC_INT64];
  double fiberAvgDoubleDepVals[NUM_SHARED_METRICS][NUM_UC_DOUBLE];
  double fiberMaxDoubleDepVals[NUM_SHARED_METRICS][NUM_UC_DOUBLE];

  int number_of_procs = 1;
  int total_threads = 0;
  MPI_COUNTS total_counts;

  // Calculate fiber values first to have thread-values for MPI stats stored
  if (analysis_flags->tasking)
    calculate_average_and_max(fiber_clocks, totalRuntimeReal,
                              fiberAvgComputation, fiberMaxComputation,
                              fiberAvgIntDepVals, fiberMaxIntDepVals,
                              fiberAvgDoubleDepVals, fiberMaxDoubleDepVals,
                              number_of_procs, total_threads, total_counts);
  calculate_average_and_max(clocks, totalRuntimeReal, avgComputation,
                            maxComputation, avgIntDepVals, maxIntDepVals,
                            avgDoubleDepVals, maxDoubleDepVals, number_of_procs,
                            total_threads, total_counts);

  // total_threads gets set in calculate_average_and_max
  int num_threads = total_threads;

  double totalRuntimeIdeal =
      (*thread_clocks)[0]->clocks[CLOCK_USEFUL].critical.getTime();

  double fiberTotalRuntimeIdeal = 0, fiberTotalOutsideOMPIdeal = 0;
  if (analysis_flags->tasking) {
    fiberTotalRuntimeIdeal =
        (fiber_clocks)[0]->clocks[CLOCK_USEFUL].critical.getTime();
  }

  // Print critical path and POP metrics (on master thread)
  if (myProcId == 0) {
    FILE *of = stdout;
    bool openedFile{false};

    if (analysis_flags->data_path) {
      if (strcmp("stdout", analysis_flags->data_path) == 0) {
        of = stdout;
      } else if (strcmp("stderr", analysis_flags->data_path) == 0) {
        of = stderr;
      } else {
        auto len = strlen(analysis_flags->data_path);
        char *tempath = (char *)malloc(len + 15);
        sprintf(tempath, "%s-%ix%i.txt", analysis_flags->data_path,
                number_of_procs, num_threads);
        of = fopen(tempath, "w");
        if (analysis_flags->verbose) {
          fprintf(analysis_flags->output, "Opened file %s for output\n",
                  tempath);
        }
        free(tempath);
        openedFile = true;
      }
    }

    if (analysis_flags->verbose) {
      fprintf(of, "\n\n--------MPI stats:--------\n");
      if (total_counts.send)
        fprintf(of, "MPI_*send: %lu\n", total_counts.send);
      if (total_counts.isend)
        fprintf(of, "MPI_I*send: %lu\n", total_counts.isend);
      if (total_counts.probe)
        fprintf(of, "MPI_*mprobe: %lu\n", total_counts.probe);
      if (total_counts.recv)
        fprintf(of, "MPI_Recv: %lu\n", total_counts.recv);
      if (total_counts.irecv)
        fprintf(of, "MPI_Irecv: %lu\n", total_counts.irecv);
      if (total_counts.coll)
        fprintf(of, "MPI_*coll: %lu\n", total_counts.coll);
      if (total_counts.coll)
        fprintf(of, "MPI_I*coll: %lu\n", total_counts.coll);
      if (total_counts.test)
        fprintf(of, "MPI_Test*: %lu\n", total_counts.test);
      if (total_counts.wait)
        fprintf(of, "MPI_Wait*: %lu\n", total_counts.wait);

      if (thread_counts) {
        fprintf(of, "\n\n--------OMPT stats:--------\n");
        auto *tCounts = (*thread_counts)[0];
        if (tCounts->taskCreate)
          fprintf(of, "taskCreate: %i\n", tCounts->taskCreate);
        if (tCounts->taskSchedule)
          fprintf(of, "taskSchedule: %i\n", tCounts->taskSchedule);
        // if(tCounts->minTaskTime)
        fprintf(of, "minTaskTime: %.9f\n", tCounts->minTaskTime);
        // if(tCounts->maxTaskTime)
        fprintf(of, "maxTaskTime: %.9f\n", tCounts->maxTaskTime);
        // if(tCounts->totalTaskTime)
        fprintf(of, "totalTaskTime: %f\n", tCounts->totalTaskTime);
        if (tCounts->totalTasks) {
          fprintf(of, "totalTasks: %i\n", tCounts->totalTasks);
          fprintf(of, "avgTaskTime: %.9f\n",
                  tCounts->totalTaskTime / tCounts->totalTasks);
        }
        // if(tCounts->minImplTaskTime)
        fprintf(of, "minImplTaskTime: %.9f\n", tCounts->minImplTaskTime);
        // if(tCounts->maxImplTaskTime)
        fprintf(of, "maxImplTaskTime: %.9f\n", tCounts->maxImplTaskTime);
        // if(tCounts->totalImplTaskTime)
        fprintf(of, "totalImplTaskTime: %f\n", tCounts->totalImplTaskTime);
        if (tCounts->totalImplTasks) {
          fprintf(of, "totalImplTasks: %i\n", tCounts->totalImplTasks);
          fprintf(of, "avgImplTaskTime: %.9f\n",
                  tCounts->totalImplTaskTime / tCounts->totalImplTasks);
        }

        if (tCounts->implTaskBegin)
          fprintf(of, "implTaskBegin: %i\n", tCounts->implTaskBegin);
        if (tCounts->implTaskEnd)
          fprintf(of, "implTaskEnd: %i\n", tCounts->implTaskEnd);
        if (tCounts->syncRegionBegin)
          fprintf(of, "syncRegionBegin: %i\n", tCounts->syncRegionBegin);
        if (tCounts->syncRegionEnd)
          fprintf(of, "syncRegionEnd: %i\n", tCounts->syncRegionEnd);
        if (tCounts->mutexAcquire)
          fprintf(of, "mutexAcquire: %i\n", tCounts->mutexAcquire);
        fprintf(analysis_flags->output, "taskCreateTime: %f\n",
                (*thread_counts)[0]->timeTaskCreate);
        fprintf(analysis_flags->output, "taskScheduleTime: %f\n",
                (*thread_counts)[0]->timeTaskSchedule);
        fprintf(analysis_flags->output, "syncTime: %f\n",
                (*thread_counts)[0]->timeSync);
        fprintf(analysis_flags->output, "dependencesTime: %f\n",
                (*thread_counts)[0]->timeDependences);
      }
    }

    // assumption: 1 parallel region, no nesting

    double taskCUEForNThreadsOld =
        std::ceil(std::ceil(avgComputation[0] * total_threads /
                            fiberTotalRuntimeIdeal) /
                  total_threads) *
        fiberTotalRuntimeIdeal;
    double totTime = avgComputation[0] * total_threads;
    double numDepChains = std::floor(totTime / fiberTotalRuntimeIdeal);
    double restChains = totTime - (numDepChains * fiberTotalRuntimeIdeal);
    double numTmin = std::ceil(numDepChains / total_threads);
    double taskCUEForNThreads = numTmin * fiberTotalRuntimeIdeal;
    if (total_threads * numTmin == numDepChains)
      taskCUEForNThreads += restChains;
    double taskCUEForNThreadsNew1 = avgComputation[0] + fiberTotalRuntimeIdeal;
    double taskCUEForNThreadsNew2 = fiberTotalRuntimeIdeal + avgComputation[0] -
                                    (fiberTotalRuntimeIdeal / total_threads);

    if (analysis_flags->verbose)
      fprintf(of,
              "totTime: %6.3lf, #depChains: %6.3lf, restChainTime: %6.3lf, "
              "numTmin: %6.3lf \n",
              totTime, numDepChains, restChains, numTmin);

    fprintf(of, "\n\n--------CritPath Analysis Tool results:--------\n");
    fprintf(of, "=> Number of processes:          %i\n", number_of_procs);
    fprintf(of, "=> Number of threads:            %i\n", total_threads);
    fprintf(of,
            "=> AVGT: Threadlocal CUE (in s): \n    Thread-centric: %6.9lf\n",
            avgComputation[0]);
    fprintf(of,
            "=> MAXT: Threadlocal CUE (in s): \n    Thread-centric: %6.9lf\n",
            maxComputation[0]);
    fprintf(of,
            "=> Global CUE (in s): \n    Thread-centric: %6.9lf\n    "
            "Task-centric: %6.9lf\n    Task-centric for %i threads: %6.9lf\n",
            totalRuntimeIdeal, fiberTotalRuntimeIdeal, total_threads,
            taskCUEForNThreads);
    fprintf(of,
            "=> AVGP: Processlocal CUE (in s): \n    Thread-centric: %6.3lf\n  "
            "  Task-centric:%6.3lf\n",
            avgComputation[1], fiberAvgComputation[1]);
    fprintf(of,
            "=> MAXP: Processlocal CUE (in s): \n    Thread-centric: %6.3lf\n  "
            "  Task-centric:%6.3lf\n",
            maxComputation[1], fiberMaxComputation[1]);
    fprintf(of, "=> Total runtime (in s):         %6.9lf\n", totalRuntimeReal);
    // fprintf(of, "=> Total runtime corrected (max1) (in s):         %6.9lf\n",
    // runtimeCorrectedMaxOverhead1); fprintf(of, "=> Total runtime corrected
    // (max2) (in s):         %6.9lf\n", runtimeCorrectedMaxOverhead2);
    // fprintf(of, "=> Total runtime corrected (avg) (in s):         %6.9lf\n",
    // runtimeCorrectedAvgOverhead); fprintf(of, "=> Total runtime no tool (sum)
    // (in s):         %6.9lf\n", runtimeNoTool); fprintf(of, "Global
    // task-centric CUE for N threads v1: %6.9lf vs:
    // %6.9lf\n",taskCUEForNThreadsNew1,taskCUEForNThreadsNew2);

    if (thread_counts) {
      auto *tCounts = (*thread_counts)[0];
      fprintf(of, "\n#expl. tasks: %i, #impl. tasks: %i\n", tCounts->totalTasks,
              tCounts->totalImplTasks);
      fprintf(of, "Expl. task times: min: %.9f, max: %.9f, avg: %.9f\n",
              tCounts->minTaskTime, tCounts->maxTaskTime,
              tCounts->totalTaskTime / tCounts->totalTasks);
      fprintf(of, "Impl. task times: min: %.9f, max: %.9f, avg: %.9f\n",
              tCounts->minImplTaskTime, tCounts->maxImplTaskTime,
              tCounts->totalImplTaskTime / tCounts->totalImplTasks);
    }
#if NUM_UC_INT64 > 0
    // Int dep metrics
    fprintf(of, "\n%i Integer Dependent Metrics: ", depOrga->getNumIntValues());
    if (depOrga->getNumIntValues() > 0) {
      for (int i = 0; i < idhs->Size(); i++) {
        for (int j = 0; j < (*idhs)[i]->getNumValues(); j++) {
          printf("'%s' ", (*idhs)[i]->getMetricName(j).c_str());
        }
      }
      fprintf(of, "\n=> AVGT: Threadlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumIntValues(); j++) {
        fprintf(of, " %f", avgIntDepVals[0][j]);
      }
      fprintf(of, "\n=> MAXT: Threadlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumIntValues(); j++) {
        fprintf(of, " %i", maxIntDepVals[0][j]);
      }
      fprintf(of, "\n=> Global CUE (in s): \n    Thread-centric: ");
      thread_local_clock->clocks[CLOCK_USEFUL].critical.printIntValues();
      fprintf(of, "    Task-centric: ");
      thread_local_clock->fiber->clocks[CLOCK_USEFUL].critical.printIntValues();
      fprintf(of, "=> AVGP: Processlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumIntValues(); j++) {
        fprintf(of, " %f", avgIntDepVals[1][j]);
      }
      fprintf(of, "\n    Task-centric: ");
      for (int j = 0; j < depOrga->getNumIntValues(); j++) {
        fprintf(of, " %f", fiberAvgIntDepVals[1][j]);
      }
      fprintf(of, "\n=> MAXP: Processlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumIntValues(); j++) {
        fprintf(of, " %i", maxIntDepVals[1][j]);
      }
      fprintf(of, "\n    Task-centric: ");
      for (int j = 0; j < depOrga->getNumIntValues(); j++) {
        fprintf(of, " %i", fiberMaxIntDepVals[1][j]);
      }
    }
#endif
#if NUM_UC_DOUBLE > 0
    // Double dep metrics
    fprintf(of,
            "\n\n%i Double Dependent Metrics: ", depOrga->getNumDoubleValues());
    if (depOrga->getNumDoubleValues() > 0) {
      for (int i = 0; i < ddhs->Size(); i++) {
        for (int j = 0; j < (*ddhs)[i]->getNumValues(); j++) {
          printf("'%s' ", (*ddhs)[i]->getMetricName(j).c_str());
        }
      }
      fprintf(of, "\n=> AVGT: Threadlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumDoubleValues(); j++) {
        fprintf(of, " %6.6f", avgDoubleDepVals[0][j]);
      }
      fprintf(of, "\n=> MAXT: Threadlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumDoubleValues(); j++) {
        fprintf(of, " %6.6f", maxDoubleDepVals[0][j]);
      }
      fprintf(of, "\n=> Global CUE (in s): \n    Thread-centric: ");
      thread_local_clock->clocks[CLOCK_USEFUL].critical.printDoubleValues();
      fprintf(of, "    Task-centric: ");
      thread_local_clock->fiber->clocks[CLOCK_USEFUL]
          .critical.printDoubleValues();
      fprintf(of, "=> AVGP: Processlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumDoubleValues(); j++) {
        fprintf(of, " %6.6f", avgDoubleDepVals[1][j]);
      }
      fprintf(of, "\n    Task-centric: ");
      for (int j = 0; j < depOrga->getNumDoubleValues(); j++) {
        fprintf(of, " %6.6f", fiberAvgDoubleDepVals[1][j]);
      }
      fprintf(of, "\n=> MAXP: Processlocal CUE (in s): \n    Thread-centric: ");
      for (int j = 0; j < depOrga->getNumDoubleValues(); j++) {
        fprintf(of, " %6.6f", maxDoubleDepVals[1][j]);
      }
      fprintf(of, "\n    Task-centric: ");
      for (int j = 0; j < depOrga->getNumDoubleValues(); j++) {
        fprintf(of, " %6.6f", fiberMaxDoubleDepVals[1][j]);
      }
    }
#endif

    // Print POP metrics
    print_POP_metrics(avgComputation, maxComputation, totalRuntimeReal,
                      runtimeNoTool, totalRuntimeIdeal, of);

    if (openedFile) {
      fclose(of);
    }
  }
}

inline int my_get_tid() {
  return thread_local_clock ? thread_local_clock->thread_id : 0;
}

void startTool(bool toolControl, ClockState cs) {
  // if stopped=0, the omp_control_tool_start callback should have no influence
  if (analysis_flags->stopped && !toolControl)
    return;
  if (!analysis_flags->running) {
    DCHECK_EQ(thread_local_clock->GetState(), STATE_INIT);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_USEFUL].thread.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_USEFUL].proc.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_USEFUL].critical.getTime(), 0);
    if (analysis_flags->tasking) {
      // checks for fiber clocks
      DCHECK_EQ(thread_local_clock->fiber->GetState(), STATE_INIT);
      DCHECK_EQ(
          thread_local_clock->fiber->clocks[CLOCK_USEFUL].thread.getTime(), 0);
      DCHECK_EQ(thread_local_clock->fiber->clocks[CLOCK_USEFUL].proc.getTime(),
                0);
      DCHECK_EQ(
          thread_local_clock->fiber->clocks[CLOCK_USEFUL].critical.getTime(),
          0);
    }

#if 0 && defined(USE_MPI)
    if (useMpi) {
      if (analysis_flags->barrier) {
        PMPI_Barrier(MPI_COMM_WORLD);
      }
    }
#endif
    if (analysis_flags->verbose)
      fprintf(analysis_flags->output, "starting tool\n");
    double time = getTime();
    analysis_flags->running = true;
    startMeasurement(time);
    // For MPI initialization
    if (cs == STATE_MPI && thread_local_clock->GetState() == STATE_INIT)
      thread_local_clock->enterState(time, STATE_USEFUL, __func__);
    thread_local_clock->enterState(time, cs, __func__);
  }
}

void stopTool() {
  // if stopped=0, the omp_control_tool_end callback should have no influence
  if (!analysis_flags->stopped)
    return;
#if 0 && defined(USE_MPI)
  if (useMpi) {
    if (analysis_flags->barrier) {
      PMPI_Barrier(MPI_COMM_WORLD);
    }
  }
#endif
  if (analysis_flags->running) {
    if (analysis_flags->verbose)
      fprintf(analysis_flags->output, "ending tool\n");
    double time = getTime();
    thread_local_clock->setState(STATE_INIT, __func__);
    analysis_flags->running = false;
    stopMeasurement(time);
  }
  if (analysis_flags->dump_on_stop) {
    finishMeasurement();
  }
}

#ifdef ATEXIT_MITIGATION
__attribute__((destructor))
#endif
void exitHandler() {
  if (analysis_flags->start_with_library_constructor &&
      analysis_flags->running) {
    if (analysis_flags->verbose)
      fprintf(analysis_flags->output, "Exiting library\n");
    finishMeasurement();
  }
}

__attribute__((constructor)) void onLibraryLoad() {
  InitializeCptFlags();
  startTimeOffset = (long long)startProgrammTime;
  startProgrammTime -= startTimeOffset;

  // Initialize library with the constructor if requested
  if (analysis_flags->start_with_library_constructor) {
    if (!analysis_flags->enabled) {
      if (analysis_flags->verbose)
        fprintf(stderr, "Tool disabled, stopping operation\n");
      return;
    }

    if (analysis_flags->verbose)
      fprintf(analysis_flags->output,
              "Starting OTF-CPT in library constructor\n");

    // pagesize = getpagesize();

    // Init with thread clocks
    if (!thread_clocks)
      thread_clocks = new Vector<THREAD_CLOCK *>{};

    // Create a dummy thread clock
    if (!thread_local_clock) {
      OmpFiberPoolInit();
      thread_local_clock =
          THREAD_CLOCK::New(my_next_id(), 0, ompt_thread_initial);
    }
    thread_clocks->PushBack(thread_local_clock);

#ifdef USE_ERRHANDLER
    init_signalhandlers();
#endif
    startMeasurement();
    thread_local_clock->enterState(startProgrammTime, STATE_USEFUL, __func__);
    // Register an exit handler to finish and print measurements later
#ifndef ATEXIT_MITIGATION
    atexit(exitHandler);
#endif
  }
}
