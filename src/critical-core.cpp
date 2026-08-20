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

#include "criticalPath.h"
#include "errorhandler.h"
#include "parse_flags.h"
#include <cstdio>
#include <time.h>
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
    PrintStack();
    _exit(signum + 128);
  }
  if (signum == SIGTERM || signum == SIGUSR2) {
    PrintStack();
    fflush(stdout);
    sleep(1);
    _exit(signum + 128);
  }
  PrintStack();

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
  struct timespec curr;
  clock_gettime(CLOCK_REALTIME, &curr);
  return curr.tv_sec - startTimeOffset + curr.tv_nsec * 1e-9 - localTimeOffset;
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
double startProgrammTime = getTime(), endProgrammTime;
double crit_path_useful_time = 0;

Vector<THREAD_CLOCK *> *thread_clocks = nullptr;
Vector<omptCounts *> *thread_counts = nullptr;
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

void resetMpiClock(THREAD_CLOCK *thread_clock) {
  // Make sure MPI clock is not started
  DCHECK_OR(thread_clock->GetState() == STATE_MPI,
            thread_clock->GetState() == STATE_INIT);
  thread_clock->clocks[CLOCK_OMPI].Reset(0);
}

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

void stopMeasurement(double time) {
  if (analysis_flags->running) {
    endProgrammTime = time;
    analysis_flags->running = false;
  }
}

template <>
double atomic_add<double>(std::atomic<double> &operand, double value_to_add) {
  double old = operand.load(std::memory_order_consume);
  double desired = old + value_to_add;
  while (!operand.compare_exchange_weak(old, desired, std::memory_order_release,
                                        std::memory_order_consume))
    desired = old + value_to_add;

  return desired;
}

#define NUM_SHARED_METRICS 7

void finishMeasurement() {
  static int finished = 0;
  if (finished)
    return;
  finished = 1;
  int number_of_procs = 1;
  int total_threads = 0;
  int num_threads = 0;
  if (thread_clocks)
    total_threads = num_threads = thread_clocks->size();

  double avgComputation[NUM_SHARED_METRICS] = {0};
  double maxComputation[NUM_SHARED_METRICS] = {0};
  double uc_avg[NUM_SHARED_METRICS] = {0};
  double uc_max[NUM_SHARED_METRICS] = {0};

  stopMeasurement();

  // STATE_INIT to stop clock
  thread_local_clock->setState(endProgrammTime, STATE_INIT, __func__);

  double totalRuntimeReal = endProgrammTime - startProgrammTime;
  if (analysis_flags->verbose) {
    printf("runtime flag: %lf, %lf\n", totalRuntimeReal,
           analysis_flags->runtime);
  }

  if (analysis_flags->runtime > 0)
    totalRuntimeReal = analysis_flags->runtime;
  // get max and avg Computation accross all threads
  MPI_COUNTS proc_counts, total_counts;
  if (num_threads > 0) {
    for (int i = 0; i < num_threads; i++) {
      auto *tclock = (*thread_clocks)[i];
      // STATE_INIT to stop all clocks
      if (tclock->GetState() != STATE_INIT)
        tclock->setState(endProgrammTime, STATE_INIT, __func__);
      proc_counts.add(*tclock);
      double curr_uc = tclock->clocks[CLOCK_USEFUL].thread.getTime();
      double curr_oot = tclock->clocks[CLOCK_OOMP].thread.getTime();
      if (curr_uc > uc_max[0]) {
        uc_max[0] = curr_uc;
      }
      if (curr_oot > uc_max[2]) {
        uc_max[2] = curr_oot;
      }
      uc_avg[0] += curr_uc;
      uc_avg[2] += curr_oot;
    }
    if (thread_counts)
      for (int i = 1; i < thread_counts->size(); i++)
        (*thread_counts)[0]->add(*(*thread_counts)[i]);
  } else {
    num_threads = 1;
    uc_max[0] = uc_avg[0] =
        thread_local_clock->clocks[CLOCK_USEFUL].thread.getTime();
    uc_max[2] = uc_avg[2] =
        thread_local_clock->clocks[CLOCK_OOMP].thread.getTime();
    proc_counts.add(*thread_local_clock);
  }
  uc_max[1] = uc_avg[1] = thread_local_clock->clocks[CLOCK_OMPI].proc.getTime();
  uc_max[3] = uc_avg[3] =
      thread_local_clock->clocks[CLOCK_USEFUL].proc.getTime();
  uc_max[4] = uc_avg[4] = thread_local_clock->clocks[CLOCK_OOMP].proc.getTime();
  uc_max[5] = uc_avg[5] = uc_avg[3] - uc_avg[4];
  uc_avg[5] = uc_avg[5] * num_threads;
  uc_max[6] = uc_max[0];
  uc_avg[6] = uc_max[0] * num_threads;

#ifdef USE_MPI
  if (useMpi) {
    // aggregate all max and avg computations on the master thread
    PMPI_Reduce(&uc_max, &maxComputation, NUM_SHARED_METRICS, MPI_DOUBLE,
                MPI_MAX, 0, MPI_COMM_WORLD);
    PMPI_Reduce(&uc_avg, &avgComputation, NUM_SHARED_METRICS, MPI_DOUBLE,
                MPI_SUM, 0, MPI_COMM_WORLD);
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
    avgComputation[3] = avgComputation[3] / number_of_procs;
    avgComputation[4] = avgComputation[4] / number_of_procs;
    avgComputation[0] = avgComputation[0] / total_threads;
    avgComputation[2] = avgComputation[2] / total_threads;
    avgComputation[5] = avgComputation[5] / total_threads;
    avgComputation[6] = avgComputation[6] / total_threads;
  } else
#endif
  {
    for (int i = 0; i < NUM_SHARED_METRICS; i++) {
      maxComputation[i] = uc_max[i];
      avgComputation[i] = uc_avg[i];
    }
    avgComputation[0] /= num_threads;
    avgComputation[2] /= num_threads;
    avgComputation[5] /= num_threads;
    avgComputation[6] /= num_threads;
  }
  if (myProcId == 0) { // display results on master thread
                       // calculate pop metrics
    double totalRuntimeIdeal =
        thread_local_clock->clocks[CLOCK_USEFUL].critical.getTime();
    double totalOutsideMPIIdeal =
        thread_local_clock->clocks[CLOCK_OMPI].critical.getTime();
    double totalOutsideOMPIdeal =
        thread_local_clock->clocks[CLOCK_OOMP].critical.getTime();

    avgComputation[5] = avgComputation[5] + totalRuntimeReal;
    maxComputation[5] = maxComputation[5] + totalRuntimeReal;

    double CommE = maxComputation[0] / totalRuntimeReal;
    double TE = totalRuntimeIdeal / totalRuntimeReal;
    double SerE = maxComputation[0] / totalRuntimeIdeal;
    double LB = avgComputation[0] / maxComputation[0];
    double PE = LB * CommE;

    double mpiLB = avgComputation[6] / maxComputation[0];
    double ompLB = LB / mpiLB;

    double ompTE = totalOutsideOMPIdeal / totalRuntimeReal;
    double mpiTE = TE / ompTE;

    double mpiSerE = maxComputation[3] / totalRuntimeIdeal;
    double ompSerE = SerE / mpiSerE;

    double mpiCommE = mpiSerE * mpiTE;
    double ompCommE = ompSerE * ompTE;

    double ompPE = ompLB * ompCommE;
    double mpiPE = mpiLB * mpiCommE;

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
      fprintf(
          of,
          "\n[pop] "
          "sere:%6.3lf:te:%6.3lf:comme:%6.3lf:lb:%6.3lf:pe:%6.3lf:crittime:%2."
          "5lf:totaltime:%6.3lf:avgcomputation:%6.3lf:maxcomputation:%6.3lf\n",
          SerE, TE, CommE, LB, PE, totalRuntimeIdeal, totalRuntimeReal,
          avgComputation[0], maxComputation[0]);
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
      }
    }

    fprintf(of, "\n\n--------CritPath Analysis Tool results:--------\n");
    fprintf(of, "=> Number of processes:          %i\n", number_of_procs);
    fprintf(of, "=> Number of threads:            %i\n", total_threads);
    fprintf(of, "=> Average Computation (in s):   %6.3lf\n", avgComputation[0]);
    if (analysis_flags->verbose) {
      fprintf(of, "=> Maximum Computation (in s):   %6.3lf\n",
              maxComputation[0]);
      fprintf(of, "=> Max crit. computation (in s): %6.3lf\n",
              totalRuntimeIdeal);
      fprintf(of, "=> Average crit. proc-local computation (in s):    %6.3lf\n",
              avgComputation[3]);
      fprintf(of, "=> Maximum crit. proc-local computation (in s):    %6.3lf\n",
              maxComputation[3]);
      fprintf(of, "=> Average crit. proc-local Outside OpenMP (in s): %6.3lf\n",
              avgComputation[4]);
      fprintf(of, "=> Maximum crit. proc-local Outside OpenMP (in s): %6.3lf\n",
              maxComputation[4]);
      fprintf(of, "=> Average proc-local rumtime (in s):   %6.3lf\n",
              avgComputation[5]);
      fprintf(of, "=> Maximum proc-local runtime (in s):   %6.3lf\n",
              maxComputation[5]);
      fprintf(of, "=> Average PL-max Computation (in s):   %6.3lf\n",
              avgComputation[6]);
      fprintf(of, "=> Maximum PL-max Computation (in s):   %6.3lf\n",
              maxComputation[6]);
      fprintf(of, "=> Average Outside OpenMP (in s):   %6.3lf\n",
              avgComputation[2]);
      fprintf(of, "=> Maximum Outside OpenMP (in s):   %6.3lf\n",
              maxComputation[2]);
      fprintf(of, "=> Max crit. Outside OpenMP (in s): %6.3lf\n",
              totalOutsideOMPIdeal);
    }
    fprintf(of, "=> Total runtime (in s):         %6.3lf\n", totalRuntimeReal);

    fprintf(of, "\n----------------POP metrics----------------\n");
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
    fprintf(of, "  MPI Parallel Efficiency:          %6.3lf\n",
            mpiPE * analysis_flags->metric_factor);
    fprintf(of, "    MPI Load Balance:               %6.3lf\n",
            mpiLB * analysis_flags->metric_factor);
    fprintf(of, "    MPI Communication Efficiency:   %6.3lf\n",
            mpiCommE * analysis_flags->metric_factor);
    fprintf(of, "      MPI Serialisation Efficiency: %6.3lf\n",
            mpiSerE * analysis_flags->metric_factor);
    fprintf(of, "      MPI Transfer Efficiency:      %6.3lf\n",
            mpiTE * analysis_flags->metric_factor);
    fprintf(of, "  OMP Parallel Efficiency:          %6.3lf\n",
            ompPE * analysis_flags->metric_factor);
    fprintf(of, "    OMP Load Balance:               %6.3lf\n",
            ompLB * analysis_flags->metric_factor);
    fprintf(of, "    OMP Communication Efficiency:   %6.3lf\n",
            ompCommE * analysis_flags->metric_factor);
    fprintf(of, "      OMP Serialisation Efficiency: %6.3lf\n",
            ompSerE * analysis_flags->metric_factor);
    fprintf(of, "      OMP Transfer Efficiency:      %6.3lf\n",
            ompTE * analysis_flags->metric_factor);
    fprintf(of, "-------------------------------------------\n");
    if (openedFile) {
      fclose(of);
    }
  }
}

inline int my_get_tid() {
  return thread_local_clock ? thread_local_clock->thread_id : 0;
}

void startTool(bool toolControl, ClockState cs) {
  if (analysis_flags->stopped && !toolControl)
    return;
  if (!analysis_flags->running) {
    DCHECK_EQ(thread_local_clock->GetState(), STATE_INIT);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_USEFUL].thread.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_USEFUL].proc.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_USEFUL].critical.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_OMPI].proc.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_OMPI].thread.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_OMPI].critical.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_OOMP].thread.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_OOMP].critical.getTime(), 0);
    DCHECK_EQ(thread_local_clock->clocks[CLOCK_OOMP].proc.getTime(), 0);

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
    stopMeasurement();
    thread_local_clock->setState(endProgrammTime, STATE_INIT, __func__);
  }
  if (analysis_flags->dump_on_stop) {
    finishMeasurement();
  }
}

#ifdef ATEXIT_MITIGATION
__attribute__((destructor))
#endif
void exitHandler() {
  if (analysis_flags->start_with_library_constructor) {
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
      fprintf(analysis_flags->output, "Starting CPT in library constructor\n");

    // pagesize = getpagesize();

    // Init with thread clocks
    if (!thread_clocks)
      thread_clocks = new Vector<THREAD_CLOCK *>{};

    // Create a dummy thread clock
    if (!thread_local_clock)
      thread_local_clock = new THREAD_CLOCK(my_next_id(), 0);
    thread_clocks->push_back(thread_local_clock);

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
