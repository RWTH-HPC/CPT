#ifndef CRITICALPATH_H
#define CRITICALPATH_H 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <inttypes.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

#if (defined __APPLE__ && defined __MACH__)
#include <dlfcn.h>
#endif

#include <omp-tools.h>
#include <omp.h>

#include "containers.h"
#include "debug.h"
#include "dependentMetrics.h"
// #include "handle-data.h"
#include "fiberpool.h"
#include "ipc-data.h"
#include "parse_flags.h"

using namespace cpt;

#ifdef DEBUG_CLOCKS
#define BUILD_DEBUG_CLOCKS(c) c
#else
#define BUILD_DEBUG_CLOCKS(c)
#endif

#define LINESTR1(file, line) file ":" #line
#define LINESTR(file, line) LINESTR1(file, line)
#define GET_FILELINE LINESTR(__FILE__, __LINE__)

#ifdef __GNUC__
#define G_GNUC_CHECK_VERSION(major, minor)                                     \
  ((__GNUC__ <= (major)) && (__GNUC_MINOR__ <= (minor)))
// Mitigation necessary for 12.3 and bellow
#if G_GNUC_CHECK_VERSION(12, 3)
#define ATEXIT_MITIGATION
#endif
#endif

enum ClockState {
  STATE_UNINIT = -1,
  STATE_INIT = 0,
  STATE_NONE = 1,
  STATE_USEFUL = 2,
  STATE_MPI = 3,
  STATE_OMP = 4,
  STATE_GPU = 5,
  STATE_LAST = 6
};

enum ClockType { CLOCK_USEFUL = 0, CLOCK_LAST = 1 };

extern const char *debug_clock_state_string[];

#define STRING_CLOCK_STATE(a) debug_clock_state_string[((int)(a) + 1)]

static const bool State[STATE_LAST][CLOCK_LAST] = {
    {false}, // INIT
    {false}, // NONE
    {true},  // USEFUL
    {false}, // MPI
    {false}, // OMP
    {false}  // GPU
}; // USEFUL

extern int myProcId;
extern bool useMpi;
extern double localTimeOffset;
extern long long startTimeOffset;
extern double startProgrammTime;
extern double crit_path_useful_time;

double getTime();
uint64_t my_next_id();
int my_get_tid();

template <typename T>
static void update_maximum(std::atomic<T> &maximum_value,
                           T const &value) noexcept {
  T prev_value = maximum_value;
  while (prev_value < value &&
         !maximum_value.compare_exchange_weak(prev_value, value)) {
  }
}

template <typename value_type>
value_type atomic_add(std::atomic<value_type> &operand,
                      value_type value_to_add);

template <>
double atomic_add<double>(std::atomic<double> &operand, double value_to_add);

template <typename value_type>
value_type atomic_add(std::atomic<value_type> &operand,
                      value_type value_to_add) {
  return operand += value_to_add;
}

template <class T> class UniqLock {
  std::unique_lock<std::mutex> u;

public:
  UniqLock(std::mutex &m);
  ~UniqLock() {}
};

class TimeMetric {
protected:
  std::atomic<double> value{0};

public:
  void maxUpdate(const TimeMetric &other) {
    update_maximum(value, other.value.load());
  }
  template <bool negative> void update(double time) {
    if constexpr (negative) {
      atomic_add(value, -time);
    } else {
      atomic_add(value, time);
    }
  }
  void Reset(double t = 0) { value.store(t); }
  TimeMetric(double t) : value(t) {}
  TimeMetric() {}
  TimeMetric(int index, const double *values) : value(values[index]) {}
  TimeMetric(int index, const depMetric *values)
      : value(values[index].fvalues[0]) {}
  TimeMetric &operator=(const TimeMetric &other) {
    if (this != &other) {
      value.store(other.value.load());
    }
    return *this;
  }
  TimeMetric &operator=(double time) {
    value.store(time);
    return *this;
  }
  void loadValues(double &values) { values = value.load(); }
  void loadValues(depMetric &values) { values.fvalues[0] = value.load(); }
  double getTime() { return value.load(); }
};

class DependentMetric {
protected:
  double refValue{0};                               // time
  Array<uint64_t, NUM_UC_INT64> intDepValues = {0}; // integer dependent metrics
  Array<double, NUM_UC_DOUBLE> doubleDepValues = {
      0}; // double dependent metrics

public:
  void maxUpdate(const DependentMetric &other) {
    if (refValue < other.refValue) {
      refValue = other.refValue;
      for (int i = 0; i < intDepValues.size(); i++) {
        intDepValues[i] = (other.intDepValues)[i];
      }
      for (int i = 0; i < doubleDepValues.size(); i++) {
        doubleDepValues[i] = (other.doubleDepValues)[i];
      }
    }
  }
  template <bool negative> void update(double time) {
    if constexpr (negative) {
      refValue -= time;
    } else {
      refValue += time;
    }
    auto depOrga = DepMetricOrganizer::getInstance();
    depOrga->template updateIntDepValues<negative>(&intDepValues);
    depOrga->template updateDoubleDepValues<negative>(&doubleDepValues);
  }
  template <bool negative> void update(const DependentMetric &ref) {
    if constexpr (negative) {
      refValue -= ref.refValue;
      intDepValues -= ref.intDepValues;
      doubleDepValues -= ref.doubleDepValues;
    } else {
      refValue += ref.refValue;
      intDepValues += ref.intDepValues;
      doubleDepValues += ref.doubleDepValues;
    }
  }
  void Reset(double t = 0) {
    refValue = t;
    intDepValues = {};
    doubleDepValues = {};
  }
  DependentMetric() {}
  DependentMetric(double t) : refValue(t) {}
  DependentMetric(const depMetric &values) : refValue(values.fvalues[0]) {
    for (int i = 0; i < intDepValues.size(); i++) {
      intDepValues[i] = values.ivalues[i];
    }
    for (int i = 0; i < doubleDepValues.size(); i++) {
      doubleDepValues[i] = values.fvalues[i + 1];
    }
  }
  DependentMetric &operator=(const DependentMetric &other) {
    if (this != &other) {
      refValue = other.refValue;
      for (int i = 0; i < intDepValues.size(); i++) {
        intDepValues[i] = (other.intDepValues)[i];
      }
      for (int i = 0; i < doubleDepValues.size(); i++) {
        doubleDepValues[i] = (other.doubleDepValues)[i];
      }
    }
    return *this;
  }
  void loadValues(depMetric &values) {
    values.fvalues[0] = refValue;
    for (int i = 0; i < intDepValues.size(); i++) {
      values.ivalues[i] = intDepValues[i];
    }
    for (int i = 0; i < doubleDepValues.size(); i++) {
      values.fvalues[i + 1] = doubleDepValues[i];
    }
  }
  double getTime() { return refValue; }
  Array<uint64_t, NUM_UC_INT64> *getIntDeps() { return &intDepValues; }
  Array<double, NUM_UC_DOUBLE> *getDoubleDeps() { return &doubleDepValues; }

  void printIntValues() {
    printf(" Ints:");
    for (int i = 0; i < intDepValues.size(); i++) {
      printf(" %lu", intDepValues[i]);
    }
    printf("\n");
  }

  void printDoubleValues() {
    printf(" Doubles:");
    for (int i = 0; i < doubleDepValues.size(); i++) {
      printf(" %f", doubleDepValues[i]);
    }
    printf("\n");
  }
  void printValues() {
    printf("Ref: %f", refValue);
    printIntValues();
    printDoubleValues();
  }
};

#if NUM_UC_INT64 > 0 || NUM_UC_DOUBLE > 0
using BaseMetric = DependentMetric;
#else
using BaseMetric = TimeMetric;
#endif

template <class T> struct syncClock;
using SYNC_CLOCK = syncClock<BaseMetric>;

template <class T> struct threadClock;
using THREAD_CLOCK = threadClock<BaseMetric>;

template <class T> struct cpClocks;
using CP_CLOCKS = cpClocks<BaseMetric>;

typedef SYNC_CLOCK ompt_tsan_clockid;

int my_get_tid();

extern thread_local THREAD_CLOCK *thread_local_clock;

#ifdef DEBUG_CLOCKS
#define CLOCK_DEBUG(a, b, c) DebugClocksRAII dcr = DebugClocksRAII(a, b, c)
inline std::mutex debugClockMutex;

class DebugClocksRAII {
  THREAD_CLOCK *tc;
  const char *loc;
  const char *func;

public:
  DebugClocksRAII(THREAD_CLOCK *_tc, const char *_loc, const char *_func);
  ~DebugClocksRAII();
};
#else
#define CLOCK_DEBUG(a, b, c)
#endif

template <class T> struct cpClocks {
  T thread{0};
  T proc{0};
  T critical{0};

  cpClocks() : thread(0), proc(0), critical(0) {}

  cpClocks(double time) : thread(time), proc(time), critical(time) {}

  cpClocks &operator=(const cpClocks &other) {
    if (this != &other) {
      thread = other.thread;
      proc = other.proc;
      critical = other.critical;
    }
    return *this;
  }

  void Reset(double time) {
    thread.Reset(time);
    proc.Reset(time);
    critical.Reset(time);
  }

  template <bool negative> void UpdateAll(double time) {
    thread.template update<negative>(time);
    proc.template update<negative>(time);
    critical.template update<negative>(time);
  }

  void OmpHBefore(cpClocks &cc) {
    proc.maxUpdate(cc.proc);
    critical.maxUpdate(cc.critical);
  }
  void OmpHAfter(cpClocks &cc) {
    cc.proc.maxUpdate(proc);
    cc.critical.maxUpdate(critical);
  }
};

template <class T> struct syncClock {
public:
  void Reset() { ResetBase(0.0); }

protected:
  ClockState sync_state{STATE_INIT};
  const char *init_loc{nullptr};
  const char *init_fileline{nullptr};
  alignas(64) std::mutex scMutex;
  alignas(64) cpClocks<T> clocks[CLOCK_LAST]{};

  void ResetBase(double time) {
    for (int i = CLOCK_USEFUL; i < CLOCK_LAST; i++)
      clocks[i].Reset(time);
    sync_state = STATE_INIT;
    init_loc = nullptr;
    init_fileline = nullptr;
  }

  void CheckIfBaseReset() {
    for (int i = CLOCK_USEFUL; i < CLOCK_LAST; i++) {
      DCHECK_EQ(clocks[i].thread.getTime(), 0);
      DCHECK_EQ(clocks[i].proc.getTime(), 0);
      DCHECK_EQ(clocks[i].critical.getTime(), 0);
    }
    DCHECK_EQ(sync_state, STATE_INIT);
    DCHECK_EQ(init_loc, nullptr);
    DCHECK_EQ(init_fileline, nullptr);
  }

  void InitBase(double _useful_computation) {
    clocks[CLOCK_USEFUL].critical = _useful_computation;
  }
  void InitBase(double _useful_computation, double _mpi_start_time) {
    clocks[CLOCK_USEFUL].critical = _useful_computation;
  }
  void InitBase() {}

public:
  syncClock(double _useful_computation) {
    clocks[CLOCK_USEFUL].critical = _useful_computation;
  }
  syncClock(double _useful_computation, double _mpi_start_time) {
    clocks[CLOCK_USEFUL].critical = _useful_computation;
  }
  syncClock() {}
  ClockState GetState() { return sync_state; }
  void CheckArc(const char *loc, THREAD_CLOCK *tc = thread_local_clock);
  void CheckArc(const char *loc, const char *fileline,
                THREAD_CLOCK *tc = thread_local_clock);
  void OmpHBefore(const char *loc, bool isFiberClock,
                  THREAD_CLOCK *tc = thread_local_clock);
  void OmpHBefore(const char *loc, const char *fileline, bool isFiberClock,
                  THREAD_CLOCK *tc = thread_local_clock);
  void OmpHAfter(const char *loc, bool isFiberClock,
                 THREAD_CLOCK *tc = thread_local_clock);
  void OmpHAfter(const char *loc, const char *fileline, bool isFiberClock,
                 THREAD_CLOCK *tc = thread_local_clock);
  void OmpCReset();
  void Print(const char *prefix1, const char *prefix2 = "",
             const char *prefix3 = "") {
    fprintf(analysis_flags->output,
            "Thread %d: "
            "%s (%p) %s%s: "
            "uct=%lf, ucp=%lf, ucc=%lf, ",
            my_get_tid(), prefix1, this, prefix2, prefix3,
            clocks[CLOCK_USEFUL].thread.getTime(),
            clocks[CLOCK_USEFUL].proc.getTime(),
            clocks[CLOCK_USEFUL].critical.getTime());
  }
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }
#ifdef USE_MPI
  friend void MpiHappensAfter(ipcData *uc, int remote);
  friend void MpiHappensAfter(ipcData &uc, int remote);
  friend ipcMetric *MpiHappensBefore(ipcData *uc, int remote);
  friend ipcMetric *MpiHappensBefore(ipcData &uc, int remote);
#endif
  friend void finishMeasurement();
};

struct MPI_COUNTS {
  uint64_t send{0}, recv{0}, isend{0}, irecv{0}, coll{0}, icoll{0}, test{0},
      wait{0}, pers{0}, probe{0};
  void add(const MPI_COUNTS &o) {
    send += o.send;
    recv += o.recv;
    isend += o.isend;
    irecv += o.irecv;
    coll += o.coll;
    icoll += o.icoll;
    test += o.test;
    wait += o.wait;
    pers += o.pers;
    probe += o.probe;
  }

  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
};

template <class T> struct omptCounts {
  int taskCreate{0};
  int taskSchedule{0};
  int totalTasks{0};
  T minTaskTime{MAXFLOAT};
  T maxTaskTime{0.0};
  T totalTaskTime{0.0};
  int totalImplTasks{0};
  T minImplTaskTime{MAXFLOAT};
  T maxImplTaskTime{0.0};
  T totalImplTaskTime{0.0};
  int implTaskBegin{0};
  int implTaskEnd{0};
  int syncRegionBegin{0};
  int syncRegionEnd{0};
  int mutexAcquire{0};
  double timeTaskCreate{0.0};
  double timeTaskSchedule{0.0};
  double timeSync{0.0};
  double timeDependences{0.0};
  void add(const omptCounts &o) {
    taskCreate += o.taskCreate;
    taskSchedule += o.taskSchedule;
    if (totalTasks == 0 && o.totalTasks == 0) {
      minTaskTime = 0.0;
    } else {
      minTaskTime = std::min(minTaskTime, o.minTaskTime);
    }
    maxTaskTime = std::max(maxTaskTime, o.maxTaskTime);
    totalTaskTime += o.totalTaskTime;
    totalTasks += o.totalTasks;
    if (totalImplTasks == 0 && o.totalImplTasks == 0) {
      minImplTaskTime = 0.0;
    } else {
      minImplTaskTime = std::min(minImplTaskTime, o.minImplTaskTime);
    }
    maxImplTaskTime = std::max(maxImplTaskTime, o.maxImplTaskTime);
    totalImplTaskTime += o.totalImplTaskTime;
    totalImplTasks += o.totalImplTasks;
    implTaskBegin += o.implTaskBegin;
    implTaskEnd += o.implTaskEnd;
    syncRegionBegin += o.syncRegionBegin;
    syncRegionEnd += o.syncRegionEnd;
    mutexAcquire += o.mutexAcquire;
    timeTaskCreate += o.timeTaskCreate;
    timeTaskSchedule += o.timeTaskSchedule;
    timeSync += o.timeSync;
    timeDependences += o.timeDependences;
  }

  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
};

template <class T>
struct threadClock : public syncClock<T>,
                     MPI_COUNTS,
                     __fiber::DataPoolEntry<threadClock<T>> {
  int thread_id{-1};
  bool openmp_thread{false};
  int thread_type{-1};
  Vector<Pair<ClockState, const char *>> clock_state_stack;
  using syncClock<T>::clocks;

  struct threadClock<T> *fiber =
      NULL; // fiber clock is exchanged when fiber switches, but updates at the
            // same time as the threadClock updates

  // "constructor" for a "real" thread threadclock
  threadClock *Init(int threadid, double _useful_computation, int _thread_type,
                    bool _openmp_thread) {

    checkIfReset();
    // initialization of inherited syncClock values
    this->InitBase(_useful_computation,
                   (!analysis_flags->running) ? 0 : -getTime());

    // initialization of threadClock specific values
    thread_id = threadid;
    openmp_thread = _openmp_thread;
    thread_type = _thread_type;
    clock_state_stack.PushBack({STATE_INIT, __PRETTY_FUNCTION__});
    fiber = NULL;
    if (analysis_flags->tasking)
      fiber = threadClock::New(_useful_computation, _openmp_thread);

    return this;
  }

  // "constructor" for a fiber threadclock
  threadClock *Init(double _useful_computation, bool _openmp_thread) {

    checkIfReset();
    // initialization of inherited syncClock values
    this->InitBase(_useful_computation,
                   (!analysis_flags->running) ? 0 : -getTime());

    // initialization of threadClock specific values
    openmp_thread = _openmp_thread;
    clock_state_stack.PushBack({STATE_INIT, __PRETTY_FUNCTION__});
    fiber = NULL;

    return this;
  }
  threadClock *Init() {
    checkIfReset();
    this->InitBase();
    clock_state_stack.PushBack({STATE_INIT, __PRETTY_FUNCTION__});
    fiber = NULL;
    return this;
  }
  threadClock *Init(const threadClock &other) {
    checkIfReset();
    this->Init(my_next_id(), 0);
    if (other.GetState() != STATE_INIT)
      clock_state_stack.PushBack(other.GetStateEntry());
    clocks[CLOCK_USEFUL] = other.clocks[CLOCK_USEFUL];

    fiber = NULL;
    if (other.fiber) {
      fiber = other.fiber;
    }
  }

  // create new "real" threadClock
  static threadClock *New(int threadid, double _useful_computation,
                          int _thread_type, bool _openmp_thread = false) {
    return __fiber::DataPoolEntry<threadClock>::New()->Init(
        threadid, _useful_computation, _thread_type, _openmp_thread);
  }

  // create new fiber threadClock
  static threadClock *New(double _useful_computation,
                          int _openmp_thread = false) {
    return __fiber::DataPoolEntry<threadClock>::New()->Init(_useful_computation,
                                                            _openmp_thread);
  }

  static threadClock *New() {
    return __fiber::DataPoolEntry<threadClock>::New()->Init();
  }

  static threadClock *New(const threadClock &other) {
    return __fiber::DataPoolEntry<threadClock>::New()->Init(other);
  }

  threadClock(__fiber::DataPool<threadClock> *dp)
      : __fiber::DataPoolEntry<threadClock<T>>(dp) {}

  void Reset() {
    this->ResetBase(0);
    thread_id = -1;
    openmp_thread = false;
    thread_type = -1;
    clock_state_stack.Reset();
    fiber = NULL;
  }

  void checkIfReset() {
    this->CheckIfBaseReset();
    DCHECK_EQ(thread_id, -1);
    DCHECK_EQ(openmp_thread, false);
    DCHECK_EQ(thread_type, -1);
    DCHECK(clock_state_stack.Empty());
    DCHECK_EQ(fiber, NULL);
  }

  void SwitchState(ClockState old_cs, ClockState new_cs, double time = 0,
                   const char *loc = NULL) {
    if (old_cs == new_cs)
      return;
    if (time == 0)
      time = getTime();

    for (int i = 0; i < CLOCK_LAST; i++) {
      if (State[old_cs][i] == State[new_cs][i]) {
        continue;
      } else if (!State[old_cs][i] && State[new_cs][i]) {
        clocks[i].template UpdateAll<true>(time);
      } else {
        clocks[i].template UpdateAll<false>(time);
      }
    }
  }

#if defined(DEBUG_CLOCKS)
  void inline printStateStack(const char *loc = "", const char *prefix = "") {
    fprintf(analysis_flags->output,
            "Thread %i: Clock State Stack at %s%s: ", thread_id, loc, prefix);
    for (auto elem : clock_state_stack) {
      fprintf(analysis_flags->output, "%s (%s) ",
              STRING_CLOCK_STATE(elem.first), elem.second);
    }
    fprintf(analysis_flags->output, "[back]\n");
  }
#endif

  void enterState(ClockState cs, const char *loc = NULL) {
    enterState(0, cs, loc);
  }

  void enterState(double time, ClockState cs, const char *loc = NULL) {
    if (!analysis_flags->running)
      return;
    CLOCK_DEBUG(this, loc, __func__);
    SwitchState(GetState(), cs, time, loc);
    clock_state_stack.PushBack({cs, loc});

    if (fiber) {
      fiber->enterState(time, cs, loc);
    }
  }

  void exitState(const char *loc = NULL, bool isRunning = true) {
    exitState(0, loc, isRunning);
    if (fiber) {
      fiber->exitState(0, loc, isRunning);
    }
  }

  void exitState(ClockState oldcs, ClockState nextcs, const char *loc = NULL,
                 bool isRunning = true) {
    if (!analysis_flags->running || !isRunning)
      return;
    DCHECK_EQ_VA(oldcs, GetState(), "\npre exitState State (",
                 STRING_CLOCK_STATE(GetState()), ", ", GetStateEntry().second,
                 "): ", loc, "\n");
    exitState(0, loc, isRunning);
    DCHECK_EQ_VA(nextcs, GetState(), "\npost exitState State (",
                 STRING_CLOCK_STATE(GetState()), ", ", GetStateEntry().second,
                 "): ", loc, "\n");

    if (fiber) {
      fiber->exitState(oldcs, nextcs, loc, isRunning);
    }
  }

  void exitState(double time, const char *loc = NULL, bool isRunning = true) {
    if (!analysis_flags->running || !isRunning)
      return;
    CLOCK_DEBUG(this, loc, __func__);
    ClockState old_cs = GetState();
    // Having STATE_INIT as anything but the bottom most element is invalid
    DCHECK_OR(clock_state_stack.Size() > 1, old_cs == STATE_INIT);
    if (old_cs == STATE_INIT)
      return;
    clock_state_stack.PopBack();
    SwitchState(old_cs, GetState(), time, loc);
  }

  void setState(ClockState cs, const char *loc = NULL) { setState(0, cs, loc); }

  void setState(double time, ClockState cs, const char *loc = NULL) {
    if (!analysis_flags->running)
      return;
    CLOCK_DEBUG(this, loc, __func__);
    SwitchState(GetState(), cs, time, loc);
    clock_state_stack.Back() = {cs, loc};

    if (fiber) {
      fiber->setState(time, cs, loc);
    }
  }

  void resetState() {
    for (int i = CLOCK_USEFUL; i < CLOCK_LAST; i++)
      clocks[i].Reset(0);

    clock_state_stack.Reset();
    clock_state_stack.PushBack({STATE_INIT, __PRETTY_FUNCTION__});

    // TODO: To which state do we want to reset fibers to? Probably depends on
    // where resetState is called (currently nowhere)
    if (fiber) {
      fiber->resetState();
    }
  }

  bool compareState(ClockState cs) const { return GetState() == cs; }

  const ClockState &GetState() const {
    return clock_state_stack.getBack().first;
  }
  const Pair<ClockState, const char *> &GetStateEntry() const {
    return clock_state_stack.getBack();
  }

  void *operator new(size_t size) { return malloc(size); }
  void *operator new(size_t size, void *where) {
    return where;
  } // should not be allowed according to C++ standard

  void operator delete(void *p) { free(p); }
};

typedef struct sync_clock_composite {
  ompt_tsan_clockid ompt_clockid;
  ompt_tsan_clockid fiber_clockid;
} sync_clock_composite;
extern Vector<THREAD_CLOCK *> *thread_clocks;
extern Vector<omptCounts<double> *> *thread_counts;
extern ompt_finalize_tool_t critical_ompt_finalize_tool;

void startTool(bool toolControl = true, ClockState cs = STATE_USEFUL);
void stopTool();
#ifndef NOHB
#define OmpHappensBefore(cv, ...)                                              \
  {                                                                            \
    DCHECK_EQ_VA(                                                              \
        thread_local_clock->GetState(), thread_local_clock->fiber->GetState(), \
        "\npre Thread State (",                                                \
        STRING_CLOCK_STATE(thread_local_clock->GetState()),                    \
        "): ", __PRETTY_FUNCTION__, "@", GET_FILELINE, ", Fiber State (",      \
        STRING_CLOCK_STATE(thread_local_clock->fiber->GetState()), ")\n");     \
    ((cv)->ompt_clockid)                                                       \
        .OmpHBefore(__PRETTY_FUNCTION__, GET_FILELINE, false, ##__VA_ARGS__);  \
    ((cv)->fiber_clockid)                                                      \
        .OmpHBefore(__PRETTY_FUNCTION__, GET_FILELINE, true, ##__VA_ARGS__);   \
    DCHECK_EQ_VA(                                                              \
        thread_local_clock->GetState(), thread_local_clock->fiber->GetState(), \
        "\npost Thread State (",                                               \
        STRING_CLOCK_STATE(thread_local_clock->GetState()),                    \
        "): ", __PRETTY_FUNCTION__, "@", GET_FILELINE, ", Fiber State (",      \
        STRING_CLOCK_STATE(thread_local_clock->fiber->GetState()), ")\n");     \
  }
#define OmpHappensAfter(cv, ...)                                               \
  {                                                                            \
    DCHECK_EQ_VA(                                                              \
        thread_local_clock->GetState(), thread_local_clock->fiber->GetState(), \
        "\npre Thread State (",                                                \
        STRING_CLOCK_STATE(thread_local_clock->GetState()),                    \
        "): ", __PRETTY_FUNCTION__, "@", GET_FILELINE, ", Fiber State (",      \
        STRING_CLOCK_STATE(thread_local_clock->fiber->GetState()), ")\n");     \
    ((cv)->ompt_clockid)                                                       \
        .OmpHAfter(__PRETTY_FUNCTION__, GET_FILELINE, false, ##__VA_ARGS__);   \
    ((cv)->fiber_clockid)                                                      \
        .OmpHAfter(__PRETTY_FUNCTION__, GET_FILELINE, true, ##__VA_ARGS__);    \
    DCHECK_EQ_VA(                                                              \
        thread_local_clock->GetState(), thread_local_clock->fiber->GetState(), \
        "\npost Thread State (",                                               \
        STRING_CLOCK_STATE(thread_local_clock->GetState()),                    \
        "): ", __PRETTY_FUNCTION__, "@", GET_FILELINE, ", Fiber State (",      \
        STRING_CLOCK_STATE(thread_local_clock->fiber->GetState()), ")\n");     \
  }
#else
#define OmpHappensBefore(cv, ...)
#define OmpHappensAfter(cv, ...)
#endif
#define OmpClockReset(cv)                                                      \
  {                                                                            \
    ((cv)->ompt_clockid).OmpCReset();                                          \
    ((cv)->fiber_clockid).OmpCReset();                                         \
  }

void startMeasurement(double time = getTime());
void stopMeasurement(double time = getTime());

void finishMeasurement();

template <class T>
void syncClock<T>::CheckArc(const char *loc, THREAD_CLOCK *tc_arg) {
  CheckArc(loc, "", tc_arg);
}

template <class T>
void syncClock<T>::CheckArc(const char *loc, const char *fileline,
                            THREAD_CLOCK *tc_arg) {
  if (sync_state == STATE_INIT) {
    sync_state = tc_arg->GetState();
    init_loc = loc;
    init_fileline = fileline;
  } else {
    DCHECK_EQ_VA(tc_arg->GetState(), sync_state, "\nInit location (",
                 STRING_CLOCK_STATE(sync_state), "): ", init_loc, "@",
                 init_fileline, "\nCurrent location (",
                 STRING_CLOCK_STATE(tc_arg->GetState()), "): ", loc, "@",
                 fileline, "\n");
  }
}

template <class T>
void syncClock<T>::OmpHBefore(const char *loc, bool isFiberClock,
                              THREAD_CLOCK *tc_arg) {
  OmpHBefore(loc, 0, isFiberClock, tc_arg);
}

template <class T>
void syncClock<T>::OmpHBefore(const char *loc, const char *fileline,
                              bool isFiberClock, THREAD_CLOCK *tc_arg) {
  if (!analysis_flags->running)
    return;
#ifdef DEBUG_HB
  printf("%s @%s: %p <- %p\n", __PRETTY_FUNCTION__, loc, this, tc_arg);
#endif
  UniqLock<T> lock(scMutex);
  THREAD_CLOCK *tc = tc_arg;
  if (isFiberClock)
    tc = tc_arg->fiber;
  this->CheckArc(loc, fileline, tc);
  clocks[CLOCK_USEFUL].OmpHBefore(tc->clocks[CLOCK_USEFUL]);
}

template <class T>
void syncClock<T>::OmpHAfter(const char *loc, bool isFiberClock,
                             THREAD_CLOCK *tc_arg) {
  OmpHAfter(loc, "", isFiberClock, tc_arg);
}

template <class T>
void syncClock<T>::OmpHAfter(const char *loc, const char *fileline,
                             bool isFiberClock, THREAD_CLOCK *tc_arg) {
  if (!analysis_flags->running)
    return;
#ifdef DEBUG_HB
  printf("%s @%s: %p -> %p\n", __PRETTY_FUNCTION__, loc, this, tc_arg);
#endif
  UniqLock<T> lock(scMutex);
  THREAD_CLOCK *tc = tc_arg;
  if (isFiberClock)
    tc = tc_arg->fiber;
  this->CheckArc(loc, fileline, tc);
  clocks[CLOCK_USEFUL].OmpHAfter(tc->clocks[CLOCK_USEFUL]);
}

template <class T> void syncClock<T>::OmpCReset() {
  if (!analysis_flags->running)
    return;
  UniqLock<T> lock(scMutex);
  clocks[CLOCK_USEFUL].Reset(-1e50);
  sync_state = STATE_INIT;
}

extern "C" void enterOpenMP(const char *loc);
extern "C" void exitOpenMP(const char *loc);

#endif