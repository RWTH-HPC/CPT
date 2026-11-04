#ifndef CRITICALPATH_H
#define CRITICALPATH_H 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <inttypes.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#if (defined __APPLE__ && defined __MACH__)
#include <dlfcn.h>
#endif

#include <omp-tools.h>
#include <omp.h>

#include "containers.h"
#include "debug.h"
#include "handle-data.h"
#include "parse_flags.h"

using namespace __cpt;

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

enum AnalysisTypes {
  AT_NONE = 0,
  AT_OMP = 1,
  AT_MPI = 2,
  AT_CUDA = 4,
  AT_CUDA_CP = 8
};

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

enum ClockType {
  CLOCK_USEFUL = 0,
  CLOCK_OMPI = 1,
  CLOCK_OOMP = 2,
  CLOCK_OGPU = 3,
  CLOCK_WGPU = 4,
  CLOCK_LAST = 5
};

extern const char *debug_clock_state_string[];

#define STRING_CLOCK_STATE(a) debug_clock_state_string[((int)(a) + 1)]

static const bool State[STATE_LAST][CLOCK_LAST] = {
    {false, false, false, false, false}, // INIT
    {false, true, true, true, false},    // NONE
    {true, true, true, true, false},     // USEFUL
    {false, false, true, true, false},   // MPI
    {false, true, false, true, false},   // OMP
    {true, true, true, false, true}      // GPU
}; // USEFUL, OMPI, OOMP, OGPU, WGPU

extern int myProcId;
// extern bool useMpi;
extern double localTimeOffset;
extern long long startTimeOffset;
extern double startProgrammTime;
extern double crit_path_useful_time;
extern u8 activeAnalyses;
extern u8 requestedAnalyses;

double getTime();
uint64_t my_next_id();
int my_get_tid();

void ParseRequestedAnalyses();

inline void ActivateAnalysis(u8 at) {
  activeAnalyses |= at & requestedAnalyses;
}

inline void DeactivateAnalysis(u8 at) { activeAnalyses &= ~at; }

inline bool CheckAnalysis(u8 at) {
  if (at == AT_NONE) {
    return activeAnalyses == AT_NONE;
  }
  return (activeAnalyses & at) != 0;
}

inline bool CheckRequestedAnalysis(u8 at) {
  if (at == AT_NONE) {
    return requestedAnalyses == AT_NONE;
  }
  return (requestedAnalyses & at) != 0;
}

struct KernelData {
  // finish simulates starting, execution, and completion of the kernel
  // finish also frees the KernelData
  virtual void finish(double dur, uint32_t streamId, uint32_t deviceID = 0) = 0;
  virtual ~KernelData() {}
  void operator delete(void *p) { free(p); }
};
// extern thread_local KernelData *thread_local_taskdata;
inline thread_local KernelData *thread_local_taskdata = nullptr;

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
  void add(double time) { atomic_add(value, time); }
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
  double refValue{0};   // time?
  uint64_t depValue{0}; // energy?

public:
  void maxUpdate(const DependentMetric &other) {
    if (refValue < other.refValue) {
      refValue = other.refValue;
      depValue = other.depValue;
    }
  }
  void add(const DependentMetric &ref) {
    refValue += ref.refValue;
    depValue += ref.depValue;
  }
  void Reset(double t = 0) {
    refValue = t;
    depValue = 0;
  }
  DependentMetric() {}
  DependentMetric(double t) : refValue(t) {}
  DependentMetric(const depMetric &values)
      : refValue(values.fvalues[0]), depValue(values.ivalues[0]) {}
  DependentMetric &operator=(const DependentMetric &other) {
    if (this != &other) {
      refValue = other.refValue;
      depValue = other.depValue;
    }
    return *this;
  }
  void loadValues(depMetric &values) {
    values.fvalues[0] = refValue;
    values.ivalues[0] = depValue;
  }
  double getTime() { return refValue; }
};

#if NUM_UC_INT64 > 0
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
  T thread_device{0};
  T proc_device{0};
  T critical_device{0};

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

  void AddAll(double time) {
    thread.add(time);
    proc.add(time);
    critical.add(time);
  }

  void OmpHBefore(cpClocks &cc) {
    proc.maxUpdate(cc.proc);
    critical.maxUpdate(cc.critical);
  }
  void OmpHAfter(cpClocks &cc) {
    cc.proc.maxUpdate(proc);
    cc.critical.maxUpdate(critical);
  }
  void GpuHBefore(cpClocks &cc) {
    proc_device.maxUpdate(cc.proc_device);
    critical_device.maxUpdate(cc.critical_device);
  }
  void GpuHAfter(cpClocks &cc) {
    cc.proc_device.maxUpdate(proc_device);
    cc.critical_device.maxUpdate(critical_device);
  }
};

template <class T> struct syncClock {
protected:
  cpClocks<T> clocks[CLOCK_LAST]{};
  ClockState sync_state{STATE_INIT};
  const char *init_loc{nullptr};
  const char *init_fileline{nullptr};
  std::mutex scMutex;

public:
  syncClock(double _useful_computation) {
    clocks[CLOCK_USEFUL].critical = _useful_computation;
  }
  // syncClock(double _useful_computation, double _mpi_start_time) {
  //   clocks[CLOCK_USEFUL].critical = _useful_computation;
  //   clocks[CLOCK_OMPI].proc = _mpi_start_time;
  //   clocks[CLOCK_OMPI].thread = _mpi_start_time;
  //   clocks[CLOCK_OMPI].critical = _mpi_start_time;
  // }
  syncClock() {}
  ClockState GetState() { return sync_state; }
  bool CheckArc(const char *loc, THREAD_CLOCK *tc = thread_local_clock);
  bool CheckArc(const char *loc, const char *fileline,
                THREAD_CLOCK *tc = thread_local_clock);
  void OmpHBefore(const char *loc, THREAD_CLOCK *tc = thread_local_clock);
  void OmpHBefore(const char *loc, const char *fileline,
                  THREAD_CLOCK *tc = thread_local_clock);
  void OmpHAfter(const char *loc, THREAD_CLOCK *tc = thread_local_clock);
  void OmpHAfter(const char *loc, const char *fileline,
                 THREAD_CLOCK *tc = thread_local_clock);
  void OmpCReset();
  void GpuHBefore(const char *loc, const char *fileline,
                  THREAD_CLOCK *tc = thread_local_clock);
  void GpuHAfter(const char *loc, const char *fileline,
                 THREAD_CLOCK *tc = thread_local_clock);
  void Print(const char *prefix1, const char *prefix2 = "",
             const char *prefix3 = "") {
    fprintf(
        analysis_flags->output,
        "Thread %d: "
        "%s (%p) %s%s: "
        "uct=%lf, ucp=%lf, ucc=%lf, "
        "omt=%lf, omp=%lf, omc=%lf, "
        "oot=%lf, oop=%lf, ooc=%lf, "
        "ogt=%lf, ogp=%lf, ogc=%lf\n",
        my_get_tid(), prefix1, this, prefix2, prefix3,
        clocks[CLOCK_USEFUL].thread.getTime(),
        clocks[CLOCK_USEFUL].proc.getTime(),
        clocks[CLOCK_USEFUL].critical.getTime(),
        clocks[CLOCK_OMPI].thread.getTime(), clocks[CLOCK_OMPI].proc.getTime(),
        clocks[CLOCK_OMPI].critical.getTime(),
        clocks[CLOCK_OOMP].thread.getTime(), clocks[CLOCK_OOMP].proc.getTime(),
        clocks[CLOCK_OOMP].critical.getTime(),
        clocks[CLOCK_OGPU].thread.getTime(), clocks[CLOCK_OGPU].proc.getTime(),
        clocks[CLOCK_OGPU].critical.getTime());
  }
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }
  friend void MpiHappensAfter(ipcData *uc, int remote);
  friend void MpiHappensAfter(ipcData &uc, int remote);
  friend ipcMetric *MpiHappensBefore(ipcData *uc, int remote);
  friend ipcMetric *MpiHappensBefore(ipcData &uc, int remote);
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

struct omptCounts {
  int taskCreate{0};
  int taskSchedule{0};
  int implTaskBegin{0};
  int implTaskEnd{0};
  int syncRegionBegin{0};
  int syncRegionEnd{0};
  int mutexAcquire{0};
  int kernelLaunch{0};
  void add(const omptCounts &o) {
    taskCreate += o.taskCreate;
    taskSchedule += o.taskSchedule;
    implTaskBegin += o.implTaskBegin;
    implTaskEnd += o.implTaskEnd;
    syncRegionBegin += o.syncRegionBegin;
    syncRegionEnd += o.syncRegionEnd;
    mutexAcquire += o.mutexAcquire;
    kernelLaunch += o.kernelLaunch;
  }

  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
};

template <class T> struct threadClock : public syncClock<T>, MPI_COUNTS {
  int thread_id{-1};
  bool openmp_thread{false};
  Vector<Pair<ClockState, const char *>> clock_state_stack;
  using syncClock<T>::clocks;

  threadClock(int threadid, double _useful_computation,
              bool _openmp_thread = false)
      : SYNC_CLOCK(_useful_computation), thread_id(threadid),
        openmp_thread(_openmp_thread) {
    clock_state_stack.push_back({STATE_INIT, __PRETTY_FUNCTION__});
  }
  threadClock() {
    clock_state_stack.push_back({STATE_INIT, __PRETTY_FUNCTION__});
  }
  threadClock(const threadClock &other) : threadClock(my_next_id(), 0) {
    if (other.GetState() != STATE_INIT)
      clock_state_stack.push_back(other.GetStateEntry());
    clocks[CLOCK_USEFUL] = other.clocks[CLOCK_USEFUL];
    clocks[CLOCK_OMPI] = other.clocks[CLOCK_OMPI];
    clocks[CLOCK_OOMP] = other.clocks[CLOCK_OOMP];
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
        clocks[i].AddAll(-time);
      } else {
        clocks[i].AddAll(time);
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
    clock_state_stack.push_back({cs, loc});
  }

  void exitState(const char *loc = NULL, bool isRunning = true) {
    exitState(0, loc, isRunning);
  }

  void exitState(ClockState oldcs, ClockState nextcs, const char *loc = NULL,
                 bool isRunning = true) {
    if (!analysis_flags->running || !isRunning)
      return;
    DCHECK_EQ(oldcs, GetState());
    exitState(0, loc, isRunning);
    DCHECK_EQ(nextcs, GetState());
  }

  void exitState(double time, const char *loc = NULL, bool isRunning = true) {
    if (!analysis_flags->running || !isRunning)
      return;
    CLOCK_DEBUG(this, loc, __func__);
    ClockState old_cs = GetState();
    // Having STATE_INIT as anything but the bottom most element is invalid
    DCHECK_OR(clock_state_stack.size() > 1, old_cs == STATE_INIT);
    if (old_cs == STATE_INIT)
      return;
    clock_state_stack.pop_back();
    SwitchState(old_cs, GetState(), time, loc);
  }

  void setState(ClockState cs, const char *loc = NULL) { setState(0, cs, loc); }

  void setState(double time, ClockState cs, const char *loc = NULL) {
    if (!analysis_flags->running && cs != STATE_INIT)
      return;
    CLOCK_DEBUG(this, loc, __func__);
    SwitchState(GetState(), cs, time, loc);
    clock_state_stack.back() = {cs, loc};
  }

  void resetState() {
    for (int i = CLOCK_USEFUL; i < CLOCK_LAST; i++)
      clocks[i].Reset(0);

    clock_state_stack.reset();
    clock_state_stack.push_back({STATE_INIT, __PRETTY_FUNCTION__});
  }

  bool compareState(ClockState cs) const { return GetState() == cs; }

  const ClockState &GetState() const {
    return clock_state_stack.getBack().first;
  }
  const Pair<ClockState, const char *> &GetStateEntry() const {
    return clock_state_stack.getBack();
  }
  void Add(double time, ClockState cs = STATE_GPU, const char *loc = NULL) {
    assert(loc);
#if DEBUG_CLOCKS
    this->Print("Adding at ", loc);
#endif
    if (!analysis_flags->running)
      return;
#if DEBUG_HB
    printf("%s (%p) @%s: %lf %i\n", __PRETTY_FUNCTION__, this, loc, time, cs);
#endif
    assert(cs == STATE_GPU);
    clocks[CLOCK_USEFUL].AddAll(time);
    clocks[CLOCK_OMPI].AddAll(time);
    clocks[CLOCK_OOMP].AddAll(time);
#if DEBUG_CLOCKS
    this->Print("Added at ", loc);
#endif
  }

  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
};

extern Vector<THREAD_CLOCK *> *omp_thread_clocks;
extern Vector<omptCounts *> *omp_thread_counts;
extern ompt_finalize_tool_t critical_ompt_finalize_tool;

THREAD_CLOCK *getOrCreateStreamClock(uint32_t streamID, uint32_t deviceID);
void GpuSyncDevice(uint32_t deviceID);
void resetMpiClock(THREAD_CLOCK *thread_clock);

void startTool(bool toolControl = true, ClockState cs = STATE_USEFUL);
void stopTool(bool cantDump = false);

#define OmpHappensBefore(cv, ...)                                              \
  (cv)->OmpHBefore(__PRETTY_FUNCTION__, GET_FILELINE, ##__VA_ARGS__)
#define OmpHappensAfter(cv, ...)                                               \
  (cv)->OmpHAfter(__PRETTY_FUNCTION__, GET_FILELINE, ##__VA_ARGS__)
#define OmpClockReset(cv) (cv)->OmpCReset()
#define GpuHappensBefore(cv, ...)                                              \
  (cv)->GpuHBefore(__func__, GET_FILELINE, ##__VA_ARGS__)
#define GpuHappensAfter(cv, ...)                                               \
  (cv)->GpuHAfter(__func__, GET_FILELINE, ##__VA_ARGS__)
#define OmpEnterRuntime() thread_local_clock->enterState(STATE_OMP, __func__)
#define OmpExitRuntime() thread_local_clock->exitState(__func__)
#define GpuEnterRuntime() thread_local_clock->enterState(STATE_GPU, __func__)
#define GpuExitRuntime() thread_local_clock->exitState(__func__)
#define GpuExecKernel(s, t) (s)->Add(t, STATE_GPU, __func__)
#define GpuExecKernelOnStreamID(s, d, t) getOrCreateStreamClock(s, d)->Add(t)

void startMeasurement(double time = 0);
void stopMeasurement(double time = getTime());

void finishMeasurement();

template <class T>
bool syncClock<T>::CheckArc(const char *loc, THREAD_CLOCK *tc_arg) {
  return CheckArc(loc, "", tc_arg);
}

template <class T>
bool syncClock<T>::CheckArc(const char *loc, const char *fileline,
                            THREAD_CLOCK *tc_arg) {
  if (sync_state == STATE_INIT) {
    sync_state = tc_arg->GetState();
    init_loc = loc;
    init_fileline = fileline;
    return true;
  }
  DCHECK_EQ_VA(tc_arg->GetState(), sync_state, "\nInit location (",
               STRING_CLOCK_STATE(sync_state), "): ", init_loc, "@",
               init_fileline, "\nCurrent location (",
               STRING_CLOCK_STATE(tc_arg->GetState()), "): ", loc, "@",
               fileline, "\n");
  return false;
}

template <class T>
void syncClock<T>::OmpHBefore(const char *loc, THREAD_CLOCK *tc_arg) {
  OmpHBefore(loc, 0, tc_arg);
}

template <class T>
void syncClock<T>::OmpHBefore(const char *loc, const char *fileline,
                              THREAD_CLOCK *tc_arg) {
  if (!analysis_flags->running)
    return;
#ifdef DEBUG_HB
  printf("%s @%s: %p <- %p\n", __PRETTY_FUNCTION__, loc, this, tc_arg);
#endif
  UniqLock<T> lock(scMutex);
  // simply copy if freshly initialized
  // otherwise started clocks are lost
  if (this->CheckArc(loc, fileline, tc_arg)) {
    clocks[CLOCK_USEFUL] = tc_arg->clocks[CLOCK_USEFUL];
    clocks[CLOCK_OOMP] = tc_arg->clocks[CLOCK_OOMP];
    clocks[CLOCK_OMPI] = tc_arg->clocks[CLOCK_OMPI];
    return;
  }
  clocks[CLOCK_USEFUL].OmpHBefore(tc_arg->clocks[CLOCK_USEFUL]);
  clocks[CLOCK_OOMP].OmpHBefore(tc_arg->clocks[CLOCK_OOMP]);
  clocks[CLOCK_OMPI].OmpHBefore(tc_arg->clocks[CLOCK_OMPI]);
}

template <class T>
void syncClock<T>::OmpHAfter(const char *loc, THREAD_CLOCK *tc_arg) {
  OmpHAfter(loc, "", tc_arg);
}

template <class T>
void syncClock<T>::OmpHAfter(const char *loc, const char *fileline,
                             THREAD_CLOCK *tc_arg) {
  if (!analysis_flags->running)
    return;
#ifdef DEBUG_HB
  printf("%s @%s: %p -> %p\n", __PRETTY_FUNCTION__, loc, this, tc_arg);
#endif
  UniqLock<T> lock(scMutex);
  this->CheckArc(loc, fileline, tc_arg);
  clocks[CLOCK_USEFUL].OmpHAfter(tc_arg->clocks[CLOCK_USEFUL]);
  clocks[CLOCK_OOMP].OmpHAfter(tc_arg->clocks[CLOCK_OOMP]);
  clocks[CLOCK_OMPI].OmpHAfter(tc_arg->clocks[CLOCK_OMPI]);
}

template <class T> void syncClock<T>::OmpCReset() {
  if (!analysis_flags->running)
    return;
  UniqLock<T> lock(scMutex);
  clocks[CLOCK_USEFUL].Reset(0);
  clocks[CLOCK_OMPI].Reset(0);
  clocks[CLOCK_OOMP].Reset(0);
  sync_state = STATE_INIT;
}

template <class T>
void syncClock<T>::GpuHBefore(const char *loc, const char *fileline,
                              THREAD_CLOCK *tc_arg) {
  if (!analysis_flags->running)
    return;
#ifdef DEBUG_HB
  printf("%s @%s: %p <- %p\n", __PRETTY_FUNCTION__, loc, this, tc_arg);
#endif
  UniqLock<T> lock(scMutex);
  this->CheckArc(loc, fileline, tc_arg);
  clocks[CLOCK_USEFUL].GpuHBefore(tc_arg->clocks[CLOCK_USEFUL]);
  clocks[CLOCK_OOMP].GpuHBefore(tc_arg->clocks[CLOCK_OOMP]);
  clocks[CLOCK_OMPI].GpuHBefore(tc_arg->clocks[CLOCK_OMPI]);
  clocks[CLOCK_OGPU].GpuHBefore(tc_arg->clocks[CLOCK_OGPU]);
}

template <class T>
void syncClock<T>::GpuHAfter(const char *loc, const char *fileline,
                             THREAD_CLOCK *tc_arg) {
  if (!analysis_flags->running)
    return;
  UniqLock<T> lock(scMutex);
  this->CheckArc(loc, fileline, tc_arg);
  clocks[CLOCK_USEFUL].GpuHAfter(tc_arg->clocks[CLOCK_USEFUL]);
  clocks[CLOCK_OOMP].GpuHAfter(tc_arg->clocks[CLOCK_OOMP]);
  clocks[CLOCK_OMPI].GpuHAfter(tc_arg->clocks[CLOCK_OMPI]);
  clocks[CLOCK_OGPU].GpuHAfter(tc_arg->clocks[CLOCK_OGPU]);
#ifdef DEBUG_HB
  printf("%s @%s: %p -> %p this->uccd=%lf, tc_arg->uccd=%lf\n",
         __PRETTY_FUNCTION__, loc, this, tc_arg,
         clocks[CLOCK_USEFUL].critical_device.getTime(),
         tc_arg->clocks[CLOCK_USEFUL].critical_device.getTime());
#endif
}

extern "C" void enterOpenMP(const char *loc);
extern "C" void exitOpenMP(const char *loc);

#ifdef USE_CUDA
struct CudaData;
using CudaDataPool = DataPool<CudaData>;

template <> __thread CudaDataPool *CudaDataPool::ThreadDataPool;

/// Data structure to store additional information for tasks.
struct CudaData final : DataPoolEntry<CudaData>, KernelData {
  /// This address is used for relationships of this kernel.
  ompt_tsan_clockid Kernel{0};

  ompt_tsan_clockid *GetKernelPtr() { return &Kernel; }

  CudaData *Init() {
    GpuHappensBefore(GetKernelPtr());
    return this;
  }

  void finish(double dur, uint32_t streamId, uint32_t deviceId = 0) {
    // finish simulates starting, execution, and completion of the kernel
    // finish also frees the KernelData
#if DEBUG_HB
    printf("%s (%lf, %i, %i)\n", __PRETTY_FUNCTION__, dur, streamId, deviceId);
#endif
    auto streamClock = getOrCreateStreamClock(streamId, deviceId);

    GpuHappensAfter(GetKernelPtr(), streamClock);
    // TODO: Fix clock context
    GpuExecKernel(streamClock, dur);
    Delete();
  }

  void Reset() {
    // Kernel.Reset();
  }

  static CudaData *New();

  CudaData(DataPool<CudaData> *dp) : DataPoolEntry<CudaData>(dp) {}

  void operator delete(void *p) { free(p); }
};
#endif

static KernelData *getKernelData() {
  if (KernelData *ret = thread_local_taskdata) {
    thread_local_taskdata = nullptr;
    return ret;
  }
#ifdef USE_CUDA
  return CudaData::New();
#endif
  return nullptr;
}

#endif
