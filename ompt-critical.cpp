/*
 * ompt-critical-fiber-restructured.cpp -- Critical path analysis runtime
 * library, build for hybrid OpenMp and MPI applications, including fiber
 * support
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

#include <algorithm>
#include <cstdio>
#include <dlfcn.h>

#include "criticalPath.h"
#include "dependentMetrics.h"
#include "errorhandler.h"
#include "fiberpool.h"
#include <mutex>
#include <omp-tools.h>
#include <omp.h>

#define OMPT_MULTIPLEX_TOOL_NAME "CPT"
#include "ompt-multiplex.h"

typedef enum cpt_x_task_property_t {
  cpt_x_task_property_name = 0x1,
  cpt_x_task_property_priority = 0x2,
  cpt_x_task_property_threadset = 0x4,
  cpt_x_task_property_created = 0x8
} cpt_x_task_property_t;

typedef ompt_set_result_t (*cpt_x_set_selective_callback_t)(
    uint64_t selection, ompt_callbacks_t event, ompt_callback_t callback);

typedef struct cpt_x_task_property_name_t {
  const char *name;
} cpt_x_task_property_name_t;

typedef struct cpt_x_task_property_priority_t {
  int priority;
} cpt_x_task_property_priority_t;

typedef void (*cpt_x_callback_task_property_t)(ompt_data_t *task_data,
                                               cpt_x_task_property_t kind,
                                               void *task_property);

#define cpt_x_callback_task_property 63

typedef enum cpt_task_flag_t {
  cpt_task_initial = 0x00000001,
  cpt_task_implicit = 0x00000002,
  cpt_task_explicit = 0x00000004,
  cpt_task_target = 0x00000008,
  cpt_task_taskwait = 0x00000010,
  cpt_task_undeferrable = 0x01000000,
  cpt_task_importing = 0x02000000,
  cpt_task_exporting = 0x04000000,
  cpt_task_undeferred = 0x08000000,
  cpt_task_untied = 0x10000000,
  cpt_task_final = 0x20000000,
  cpt_task_mergeable = 0x40000000,
  cpt_task_merged = 0x80000000
} cpt_task_flag_t;

typedef enum cpt_sync_region_t {
  cpt_sync_region_barrier = 1,
  cpt_sync_region_barrier_implicit = 2,
  cpt_sync_region_barrier_explicit = 3,
  cpt_sync_region_barrier_implementation = 4,
  cpt_sync_region_taskwait = 5,
  cpt_sync_region_taskgroup = 6,
  cpt_sync_region_reduction = 7,
  cpt_sync_region_barrier_implicit_workshare = 8,
  cpt_sync_region_barrier_implicit_parallel = 9,
  cpt_sync_region_barrier_teams = 10,
  cpt_sync_region_undeferrable_task = 11
} cpt_sync_region_t;

// Define attribute that indicates that the fall through from the previous
// case label is intentional and should not be diagnosed by a compiler
//   Code from libcxx/include/__config
// Use a function like macro to imply that it must be followed by a semicolon
#if __cplusplus > 201402L && __has_cpp_attribute(fallthrough)
#define KMP_FALLTHROUGH() [[fallthrough]]
// icc cannot properly tell this attribute is absent so force off
#elif defined(__INTEL_COMPILER)
#define KMP_FALLTHROUGH() ((void)0)
#elif __has_cpp_attribute(clang::fallthrough)
#define KMP_FALLTHROUGH() [[clang::fallthrough]]
#elif __has_attribute(fallthrough) || __GNUC__ >= 7
#define KMP_FALLTHROUGH() __attribute__((__fallthrough__))
#else
#define KMP_FALLTHROUGH() ((void)0)
#endif

/// Required OMPT inquiry functions.
static ompt_get_parallel_info_t critical_ompt_get_parallel_info{};
static ompt_get_thread_data_t critical_ompt_get_thread_data{};

static int hasTaskCreation = 0;
static int hasTaskProperty = 0;

static __thread omptCounts<double> *omptThreadCount;

struct ompTimer {
  const char *loc;
  ompTimer(const char *loc = NULL) : loc(loc) {
    if (thread_local_clock == nullptr) {
      OmpFiberPoolInit();
      thread_local_clock =
          THREAD_CLOCK::New(my_next_id(), 0, ompt_thread_unknown);
      // printf("Thread %i: Created thread_local_clock %p in ompTimer with fiber
      // %p\n", omp_get_thread_num(), thread_local_clock,
      // thread_local_clock->fiber);
    }
    thread_local_clock->enterState(STATE_OMP, loc);
  }
  ~ompTimer() { thread_local_clock->exitState(loc); }
};

// type: 0=task create, 1=task schedule, 2=sync
struct overheadTimer {
  int type;
  double time;
  overheadTimer(int _type) {
    type = _type;
    time = -getTime();
  }
  ~overheadTimer() {
    time += getTime();
    if (omptThreadCount && type >= 0 && type < 4) {
      double *counts[] = {
          &omptThreadCount->timeTaskCreate, &omptThreadCount->timeTaskSchedule,
          &omptThreadCount->timeSync, &omptThreadCount->timeDependences};
      *counts[type] += time;
    }
  }
};

static int pagesize{0};

// Data structure to provide a threadsafe pool of reusable objects.
// DataPool<Type of objects>
template <typename T> struct DataPool final {
  static __thread DataPool<T> *ThreadDataPool;
  std::mutex DPMutex{};

  // store unused objects
  Vector<T *> DataPointer{};
  Vector<T *> RemoteDataPointer{};

  // store all allocated memory to finally release
  Vector<void *> memory;

  // count remotely returned data (RemoteDataPointer.size())
  std::atomic<int> remote{0};

  // totally allocated data objects in pool
  int total{0};
#ifdef DEBUG_DATA
  int remoteReturn{0};
  int localReturn{0};

  int getRemote() { return remoteReturn + remote; }
  int getLocal() { return localReturn; }
#endif
  int getTotal() { return total; }
  int getMissing() {
    return total - DataPointer.Size() - RemoteDataPointer.Size();
  }

  // fill the pool by allocating a page of memory
  void newDatas() {
    if (remote > 0) {
      const std::lock_guard<std::mutex> lock(DPMutex);
      // DataPointer is empty, so just swap the vectors
      DataPointer.Swap(RemoteDataPointer);
      remote = 0;
      return;
    }
    // calculate size of an object including padding to cacheline size
    size_t elemSize = sizeof(T);
    size_t paddedSize = (((elemSize - 1) / 64) + 1) * 64;
    // round up to next multiple of pagesize
    int allocSize = (paddedSize + pagesize - 1) & ~(pagesize - 1);
    // number of padded elements to allocate
    int ndatas = allocSize / paddedSize;
    if (ndatas < 4)
      ndatas = 4;
    char *datas = (char *)malloc(ndatas * paddedSize);
    memory.PushBack(datas);
    for (int i = 0; i < ndatas; i++) {
      DataPointer.PushBack(new (datas + i * paddedSize) T(this));
    }
    total += ndatas;
  }

  // get data from the pool
  T *getData() {
    T *ret;
    if (DataPointer.Empty())
      newDatas();
    ret = DataPointer.Back();
    DataPointer.PopBack();
    return ret;
  }

  // accesses to the thread-local datapool don't need locks
  void returnOwnData(T *data) {
    DataPointer.PushBack(data);
#ifdef DEBUG_DATA
    localReturn++;
#endif
  }

  // returning to a remote datapool using lock
  void returnData(T *data) {
    const std::lock_guard<std::mutex> lock(DPMutex);
    RemoteDataPointer.PushBack(data);
    remote++;
#ifdef DEBUG_DATA
    remoteReturn++;
#endif
  }

  ~DataPool() {
    // we assume all memory is returned when the thread finished / destructor is
    // called
    if (analysis_flags->report_data_leak && getMissing() != 0) {
      fprintf(stderr,
              "ERROR: While freeing DataPool (%s) we are missing %i data "
              "objects.\n",
              __PRETTY_FUNCTION__, getMissing());
      exit(-3);
    }
    for (auto i : DataPointer)
      if (i)
        i->~T();
    for (auto i : RemoteDataPointer)
      if (i)
        i->~T();
    for (auto i : memory)
      if (i)
        free(i);
  }
  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
};

template <typename T> struct DataPoolEntry {
  DataPool<T> *owner;

  static T *New() { return DataPool<T>::ThreadDataPool->getData(); }

  void Delete() {
    static_cast<T *>(this)->Reset();
    if (owner == DataPool<T>::ThreadDataPool)
      owner->returnOwnData(static_cast<T *>(this));
    else
      owner->returnData(static_cast<T *>(this));
  }

  DataPoolEntry(DataPool<T> *dp) : owner(dp) {}
};

struct DependencyData;
typedef DataPool<DependencyData> DependencyDataPool;
template <>
__thread DependencyDataPool *DependencyDataPool::ThreadDataPool = nullptr;

/// Data structure to store additional information for task dependency.
struct DependencyData final : DataPoolEntry<DependencyData> {
  sync_clock_composite in;
  sync_clock_composite out;
  sync_clock_composite inoutset;
  sync_clock_composite *GetInPtr() { return &in; }
  sync_clock_composite *GetOutPtr() { return &out; }
  sync_clock_composite *GetInoutsetPtr() { return &inoutset; }

  void Reset() {}

  static DependencyData *New() { return DataPoolEntry<DependencyData>::New(); }

  DependencyData(DataPool<DependencyData> *dp)
      : DataPoolEntry<DependencyData>(dp) {}
};

struct TaskDependency {
  sync_clock_composite *inPtr;
  sync_clock_composite *outPtr;
  sync_clock_composite *inoutsetPtr;
  ompt_dependence_type_t type;
  TaskDependency(DependencyData *depData, ompt_dependence_type_t type)
      : inPtr(depData->GetInPtr()), outPtr(depData->GetOutPtr()),
        inoutsetPtr(depData->GetInoutsetPtr()), type(type) {}
  void AnnotateBegin() {
    // update threadlocal clocks from global clocks (take maximum of both)
    if (type == ompt_dependence_type_out ||
        type == ompt_dependence_type_inout ||
        type == ompt_dependence_type_mutexinoutset) {
      OmpHappensAfter(inPtr);
      OmpHappensAfter(outPtr);
      OmpHappensAfter(inoutsetPtr);
    } else if (type == ompt_dependence_type_in) {
      OmpHappensAfter(outPtr);
      OmpHappensAfter(inoutsetPtr);
    } else if (type == ompt_dependence_type_inoutset) {
      OmpHappensAfter(inPtr);
      OmpHappensAfter(outPtr);
    }
  }
  void AnnotateEnd() {
    // update outPtr, inPtr, inoutset sync clocks from threadlocal clocks (take
    // maximum of both)
    if (type == ompt_dependence_type_out ||
        type == ompt_dependence_type_inout ||
        type == ompt_dependence_type_mutexinoutset) {
      OmpHappensBefore(outPtr);
    } else if (type == ompt_dependence_type_in) {
      OmpHappensBefore(inPtr);
    } else if (type == ompt_dependence_type_inoutset) {
      OmpHappensBefore(inoutsetPtr);
    }
  }
};

struct TaskData;
struct ParallelData;
typedef DataPool<ParallelData> ParallelDataPool;
template <>
__thread ParallelDataPool *ParallelDataPool::ThreadDataPool = nullptr;

/// Data structure to store additional information for parallel regions.
struct ParallelData final : DataPoolEntry<ParallelData> {
  TaskData *encounteringTask;
  // Parallel fork is just another barrier, use Barrier[1]

  /// Three addresses for relationships with barriers.
  sync_clock_composite Barrier[3];

  const void *codePtr;

  sync_clock_composite *GetParallelPtr() { return &(Barrier[2]); }

  sync_clock_composite *GetBarrierPtr(unsigned Index) {
    return &(Barrier[Index]);
  }

  ParallelData *Init(const void *codeptr, TaskData *encounteringTask) {
    this->encounteringTask = encounteringTask;
    this->codePtr = codeptr;
    return this;
  }

  void Reset() {
    Barrier[0].fiber_clockid.Reset();
    Barrier[1].fiber_clockid.Reset();
    Barrier[2].fiber_clockid.Reset();
  }

  static ParallelData *New(const void *codeptr,
                           TaskData *encounteringTask = nullptr) {
    return DataPoolEntry<ParallelData>::New()->Init(codeptr, encounteringTask);
  }

  ParallelData(DataPool<ParallelData> *dp) : DataPoolEntry<ParallelData>(dp) {}
};

static inline ParallelData *ToParallelData(ompt_data_t *parallel_data) {
  return reinterpret_cast<ParallelData *>(parallel_data->ptr);
}

struct Taskgroup;
typedef DataPool<Taskgroup> TaskgroupPool;
template <> __thread TaskgroupPool *TaskgroupPool::ThreadDataPool = nullptr;

/// Data structure to support stacking of taskgroups and allow synchronization.
struct Taskgroup final : DataPoolEntry<Taskgroup> {
  /// Its address is used for relationships of the taskgroup's task set.
  sync_clock_composite Ptr;

  /// Reference to the parent taskgroup.
  Taskgroup *Parent;

  sync_clock_composite *GetPtr() { return &Ptr; }

  Taskgroup *Init(Taskgroup *parent) {
    Parent = parent;
    return this;
  }

  void Reset() { Ptr.fiber_clockid.Reset(); }

  static Taskgroup *New(Taskgroup *Parent) {
    return DataPoolEntry<Taskgroup>::New()->Init(Parent);
  }

  Taskgroup(DataPool<Taskgroup> *dp) : DataPoolEntry<Taskgroup>(dp) {}
};

enum CPTTaskFlag { CPTTaskFulfilled = 0x00010000 };

typedef DataPool<TaskData> TaskDataPool;
template <> __thread TaskDataPool *TaskDataPool::ThreadDataPool = nullptr;

enum CptTaskFlag { CptTaskFulfilled = 0x00010000 };

/// Data structure to store additional information for tasks.
struct TaskData final : DataPoolEntry<TaskData> {
  /// Its address is used for relationships of this task.
  sync_clock_composite Task{0};

  /// Child tasks use its address to declare a relationship to a taskwait in
  /// this task.
  sync_clock_composite Taskwait{0};

  /// Index of which barrier to use next.
  char BarrierIndex{0};

  /// Whether this task was started while analysis_flags->running == true
  bool isRunning{false};

  /// Whether this task is currently executing a barrier.
  bool InBarrier{false};
  bool InCreate{false};

  /// Whether this task is an included task.
  int TaskType{0};

  int ThreadNum{-1};

  /// count execution phase
  int execution{0};
  int priority{0};
  std::string name = "";

  size_t PrivateDataSize{0};
  void *PrivateDataAddr{nullptr};

  const void *CodePtr{nullptr};

  /// Count how often this structure has been put into child tasks + 1.
  std::atomic_int RefCount{1};

  /// Reference to the parent that created this task.
  TaskData *Parent{nullptr};

  /// Reference to the implicit task in the stack above this task.
  TaskData *ImplicitTask{nullptr};

  /// Reference to the team of this task.
  ParallelData *Team{nullptr};

  /// Reference to the current taskgroup that this task either belongs to or
  /// that it just created.
  Taskgroup *TaskGroup{nullptr};

  /// Dependency information for this task.
  TaskDependency *Dependencies{nullptr};

  /// Number of dependency entries.
  unsigned DependencyCount{0};

  // The dependency-map stores DependencyData objects representing
  // the dependency variables used on the sibling tasks created from
  // this task
  // We expect a rare need for the dependency-map, so alloc on demand
  CompactHashMap<void *, DependencyData *> *DependencyMap{nullptr};

#ifdef DEBUG
  int freed{0};
#endif

  void *Fiber{nullptr};

  void activate() {
#ifndef EMPTYCB
    if (analysis_flags->collect_task_type_share)
      TaskTypeMetricHandler::getInstance()->switchTaskType(TaskType);
    if (strcmp(analysis_flags->task_prio_shares, "") > 0)
      TaskPrioMetricHandler::getInstance()->switchTaskPrio(priority);
    if (strcmp(analysis_flags->task_name_shares, "") > 0) {
      TaskNameMetricHandler::getInstance()->switchTaskName(name);
    }
    if (Fiber)
      OmpSwitchToFiber(Fiber, 1);
    // first time that task gets scheduled
    if (thread_local_clock->fiber->GetState() == STATE_INIT) {
      thread_local_clock->fiber->enterState(thread_local_clock->GetState());
    }
#endif
    BUILD_DEBUG_CLOCKS(thread_local_clock->printStateStack();
                       thread_local_clock->fiber->printStateStack();)
  }
  void deactivate() {
    BUILD_DEBUG_CLOCKS(thread_local_clock->printStateStack();
                       thread_local_clock->fiber->printStateStack();)
#ifndef EMPTYCB
    if (Fiber != OmpGetCurrentFiber()) {
      printf("!!!!!!Fibers not matching on thread %i: %p (expected) vs %p "
             "(stored)!!!!!!\n",
             omp_get_thread_num(), Fiber, OmpGetCurrentFiber());
    }
    DCHECK_OR(!Fiber, Fiber == OmpGetCurrentFiber());
#endif
  }
  bool isUndeferrable() { return TaskType & cpt_task_undeferrable; }
  bool isUndeferred() { return TaskType & cpt_task_undeferred; }
  bool isUntied() { return TaskType & cpt_task_untied; }
  bool isFinal() { return TaskType & cpt_task_final; }
  bool isFinalUndeferred() {
    return TaskType & cpt_task_final && TaskType & cpt_task_undeferred;
  }
  bool isMergable() { return TaskType & cpt_task_mergeable; }
  bool isMerged() { return TaskType & cpt_task_merged; }

  bool isExplicit() { return TaskType & cpt_task_explicit; }
  bool isImplicit() { return TaskType & cpt_task_implicit; }
  bool isInitial() { return TaskType & cpt_task_initial; }
  bool isTarget() { return TaskType & cpt_task_target; }

  bool isFulfilled() { return TaskType & CptTaskFulfilled; }
  void setFulfilled() { TaskType |= CptTaskFulfilled; }

  sync_clock_composite *GetTaskPtr() { return &Task; }

  sync_clock_composite *GetTaskwaitPtr() { return &Taskwait; }

  TaskData *Init(TaskData *parent, int taskType, const void *codePtr) {
    CodePtr = codePtr;
    isRunning = analysis_flags->running;
    TaskType = taskType;
    Parent = parent;
    Team = Parent->Team;
    if (Parent != nullptr) {
      Parent->RefCount++;
      // Copy over pointer to taskgroup. This task may set up its own stack
      // but for now belongs to its parent's taskgroup.
      TaskGroup = Parent->TaskGroup;
#ifndef EMPTYCB
      if (analysis_flags->tasking) // && !isIncluded() && !isUntied())
        Fiber = OmpCreateFiber(0);
#endif
    }
    return this;
  }

  // initial task
  TaskData *Init(ParallelData *team, int taskType) {
    isRunning = analysis_flags->running;
    TaskType = taskType;
    execution = 1;
    ImplicitTask = this;
    Team = team;
#ifndef EMPTYCB
    if (analysis_flags->tasking)
      Fiber = OmpGetCurrentFiber();
#endif
    return this;
  }

  // implicit task
  TaskData *Init(ParallelData *team, int threadNum, int taskType) {
    isRunning = analysis_flags->running;
    TaskType = taskType;
    execution = 1;
    ImplicitTask = this;
    Team = team;
    ThreadNum = threadNum;
#ifndef EMPTYCB
    if (analysis_flags->tasking) {
      if (omp_get_thread_num() == 0 && !(taskType & cpt_task_initial)) {
        Fiber = OmpCreateFiber(0);
      } else {
        Fiber = OmpGetCurrentFiber();
      }
    }
#endif
    return this;
  }

  void Reset() {
#ifndef EMPTYCB
    if (analysis_flags->tasking && ImplicitTask != this && Fiber) {
      OmpDestroyFiber(Fiber);
    }
#endif
    Task.fiber_clockid.Reset();
    Taskwait.fiber_clockid.Reset();
    CodePtr = nullptr;
    isRunning = false;
    InBarrier = false;
    TaskType = 0;
    execution = 0;
    BarrierIndex = 0;
    RefCount = 1;
    Parent = nullptr;
    ImplicitTask = nullptr;
    Team = nullptr;
    ThreadNum = -1;
    TaskGroup = nullptr;
    PrivateDataSize = 0;
    PrivateDataAddr = nullptr;
    if (DependencyMap) {
      for (auto i : *DependencyMap)
        i.second->Delete();
      delete DependencyMap;
    }
    DependencyMap = nullptr;
    if (Dependencies)
      free(Dependencies);
    Dependencies = nullptr;
    DependencyCount = 0;
#ifdef DEBUG
    freed = 0;
#endif
  }

  static TaskData *New(TaskData *parent, int taskType,
                       const void *codePtr = nullptr) {
    return DataPoolEntry<TaskData>::New()->Init(parent, taskType, codePtr);
  }

  static TaskData *New(ParallelData *team, int taskType) {
    return DataPoolEntry<TaskData>::New()->Init(team, taskType);
  }

  static TaskData *New(ParallelData *team, int threadNum, int taskType) {
    return DataPoolEntry<TaskData>::New()->Init(team, threadNum, taskType);
  }

  TaskData(DataPool<TaskData> *dp) : DataPoolEntry<TaskData>(dp) {}
};

static inline TaskData *ToTaskData(ompt_data_t *task_data) {
  if (task_data)
    return reinterpret_cast<TaskData *>(task_data->ptr);
  return nullptr;
}

/*
 * Threads
 */

/// Store a mutex for each wait_id to resolve race condition with callbacks.
CompactHashMap<ompt_wait_id_t, Pair<std::mutex, sync_clock_composite> *> Locks;
std::mutex LocksMutex;
std::mutex tcmutex;

static void ompt_tsan_thread_begin(ompt_thread_t thread_type,
                                   ompt_data_t *thread_data) {
#ifndef EMPTYCB
  ParallelDataPool::ThreadDataPool = new ParallelDataPool;
  TaskgroupPool::ThreadDataPool = new TaskgroupPool;
  TaskDataPool::ThreadDataPool = new TaskDataPool;
  DependencyDataPool::ThreadDataPool = new DependencyDataPool;
#endif
  if (analysis_flags->tasking) {
    OmpFiberPoolInit();
  }

  omptThreadCount = new omptCounts<double>();
  if (!thread_local_clock) {
    thread_local_clock =
        THREAD_CLOCK::New(my_next_id(), 0.0, thread_type, true);
  }
  thread_data->ptr = thread_local_clock;
  {
    const std::lock_guard<std::mutex> lock(tcmutex);
    thread_clocks->PushBack(thread_local_clock);
    thread_counts->PushBack(omptThreadCount);
  }
  thread_local_clock->enterState(startProgrammTime, STATE_OMP, __func__);
}

static void ompt_tsan_thread_end(ompt_data_t *thread_data) {
#ifndef EMPTYCB
  delete ParallelDataPool::ThreadDataPool;
  delete TaskgroupPool::ThreadDataPool;
  delete TaskDataPool::ThreadDataPool;
  delete DependencyDataPool::ThreadDataPool;
#endif
}

/// OMPT event callbacks for handling parallel regions.

static void ompt_tsan_parallel_begin(ompt_data_t *parent_task_data,
                                     const ompt_frame_t *parent_task_frame,
                                     ompt_data_t *parallel_data,
                                     uint32_t requested_team_size, int flag,
                                     const void *codeptr_ra) {
#ifndef EMPTYCB
  ParallelData *Data =
      ParallelData::New(codeptr_ra, ToTaskData(parent_task_data));
  parallel_data->ptr = Data;

  // end of computation
  thread_local_clock->enterState(STATE_OMP, __func__);
  OmpHappensBefore(Data->GetParallelPtr());
#endif
}

static void ompt_tsan_parallel_end(ompt_data_t *parallel_data,
                                   ompt_data_t *task_data, int flag,
                                   const void *codeptr_ra) {
#ifndef EMPTYCB
  ParallelData *Data = ToParallelData(parallel_data);
  OmpHappensAfter(Data->GetParallelPtr());
  thread_local_clock->exitState(__func__);

  Data->Delete();
#endif
}

static void ompt_tsan_implicit_task(ompt_scope_endpoint_t endpoint,
                                    ompt_data_t *parallel_data,
                                    ompt_data_t *task_data,
                                    unsigned int team_size,
                                    unsigned int thread_num, int type) {
#ifndef EMPTYCB
  switch (endpoint) {
  case ompt_scope_begin:
    // printf("In ompt_scope_begin of impl task on thread %i, running=%i\n",
    // omp_get_thread_num(), analysis_flags->running);
    if (analysis_flags->running)
      omptThreadCount->implTaskBegin++;
    if (type & cpt_task_initial) {
      parallel_data->ptr = ParallelData::New(nullptr);
    } else {
      // In case of reusing OMP threads from a parallel region
      // created when the tool was not yet running
      if (thread_local_clock->GetState() == STATE_INIT) {
        thread_local_clock->enterState(STATE_OMP, "ImplicitTaskBegin");
      }
    }
    TaskData *implTask;
    task_data->ptr = implTask =
        TaskData::New(ToParallelData(parallel_data), thread_num, type);
    if (!(type & cpt_task_initial)) {
      if (thread_num == 0) {
        ToParallelData(parallel_data)->encounteringTask->deactivate();
        implTask->activate();
      } else {
        TaskTypeMetricHandler::getInstance()->switchTaskType(type);
      }
      OmpHappensAfter(ToParallelData(parallel_data)->GetParallelPtr());
    }
    thread_local_clock->enterState(STATE_USEFUL, "ImplicitTaskBegin");
    break;
  case ompt_scope_end: {
    // printf("In ompt_scope_end of impl task on thread %i, running=%i\n",
    // omp_get_thread_num(), analysis_flags->running); end of the task, useful
    // computation stop
    TaskData *Data = ToTaskData(task_data);
#ifdef DEBUG
    DCHECK(Data->freed == 0 && "Implicit task end should only be called once!");
    Data->freed = 1;
#endif
    thread_local_clock->exitState("ImplicitTaskEnd", Data->isRunning);
    if (Data->isRunning) {
      omptThreadCount->implTaskEnd++;
      if (analysis_flags->tasking) {
        THREAD_CLOCK *fiber = reinterpret_cast<THREAD_CLOCK *>(Data->Fiber);
        double taskTime = fiber->clocks[CLOCK_USEFUL].thread.getTime();
        omptThreadCount->totalImplTasks++;
        omptThreadCount->totalImplTaskTime += taskTime;
        omptThreadCount->minImplTaskTime =
            std::min(omptThreadCount->minImplTaskTime, taskTime);
        omptThreadCount->maxImplTaskTime =
            std::max(omptThreadCount->maxImplTaskTime, taskTime);
      }
    }

    if (!(type & cpt_task_initial) && thread_num == 0) {
      auto *implTask = ToTaskData(task_data);
      OmpHappensBefore(implTask->Team->GetParallelPtr());
      implTask->deactivate();
      implTask->Team->encounteringTask->activate();
    }
    DCHECK(Data->RefCount == 1 &&
           "All tasks should have finished at the implicit barrier!");
    Data->Delete();
    if (type & cpt_task_initial) {
      ToParallelData(parallel_data)->Delete();
    }
    break;
  }
  case ompt_scope_beginend:
    // Should not occur according to OpenMP 5.1
    // Tested in OMPT tests
    break;
  }
#endif
}

static void ompt_tsan_sync_region(ompt_sync_region_t kind,
                                  ompt_scope_endpoint_t endpoint,
                                  ompt_data_t *parallel_data,
                                  ompt_data_t *task_data,
                                  const void *codeptr_ra) {
#ifndef EMPTYCB
  overheadTimer ovT{2};
  TaskData *Data = ToTaskData(task_data);
  switch (endpoint) {
  case ompt_scope_begin:
  case ompt_scope_beginend:
    if (analysis_flags->running)
      omptThreadCount->syncRegionBegin++;
    // runtime overhead, stop useful
    switch ((cpt_sync_region_t)kind) {
    case cpt_sync_region_barrier_implementation:
    case cpt_sync_region_barrier_implicit:
    case cpt_sync_region_barrier_explicit:
    case cpt_sync_region_barrier_implicit_parallel:
    case cpt_sync_region_barrier_implicit_workshare:
    case cpt_sync_region_barrier_teams:
    case cpt_sync_region_barrier: {
      DCHECK_NOT(Data->InCreate);
      Data->InBarrier = true;
      thread_local_clock->enterState(STATE_OMP, "SyncRegionBegin");
      char BarrierIndex = Data->BarrierIndex;
      OmpHappensBefore(Data->Team->GetBarrierPtr(BarrierIndex));
      if (Data->ThreadNum == 0)
        OmpClockReset(Data->Team->GetBarrierPtr((BarrierIndex + 1) % 3));
      break;
    }

    case cpt_sync_region_undeferrable_task:
    case cpt_sync_region_taskwait: {
      DCHECK_NOT(Data->InCreate);
      DCHECK_NOT(Data->InBarrier);
      Data->InBarrier = true;
      thread_local_clock->enterState(STATE_OMP, "SyncRegionBegin");
      break;
    }

    case cpt_sync_region_taskgroup:
      Data->TaskGroup = Taskgroup::New(Data->TaskGroup);
      break;

    case cpt_sync_region_reduction:
      // should never be reached
      break;
    }
    if (endpoint == ompt_scope_begin)
      break;
    KMP_FALLTHROUGH();
  case ompt_scope_end:
    if (analysis_flags->running)
      omptThreadCount->syncRegionEnd++;
    switch ((cpt_sync_region_t)kind) {
    case cpt_sync_region_barrier_implementation:
    case cpt_sync_region_barrier_implicit:
    case cpt_sync_region_barrier_explicit:
    case cpt_sync_region_barrier_implicit_parallel:
    case cpt_sync_region_barrier_implicit_workshare:
    case cpt_sync_region_barrier_teams:
    case cpt_sync_region_barrier: {
      char BarrierIndex = Data->BarrierIndex;
      // Barrier will end after it has been entered by all threads.
      if (parallel_data || Data->ThreadNum == 0)
        OmpHappensAfter(Data->Team->GetBarrierPtr(BarrierIndex));

      // It is not guaranteed that all threads have exited this barrier before
      // we enter the next one. So we will use a different address.
      // We are however guaranteed that this current barrier is finished
      // by the time we exit the next one. So we can then reuse the first
      // address.
      Data->BarrierIndex = (BarrierIndex + 1) % 3;
      Data->InBarrier = false;
      thread_local_clock->exitState("SyncRegionEnd", Data->isRunning);
      break;
    }

    case cpt_sync_region_taskwait: {
      if (Data->execution > 1)
        OmpHappensAfter(Data->GetTaskwaitPtr());
      Data->InBarrier = false;
      thread_local_clock->exitState("SyncRegionEnd", Data->isRunning);
      break;
    }

    case cpt_sync_region_undeferrable_task: {
      OmpHappensAfter(Data->GetTaskPtr());
      Data->InBarrier = false;
      thread_local_clock->exitState("SyncRegionEnd", Data->isRunning);
      break;
    }

    case cpt_sync_region_taskgroup: {
      DCHECK(Data->TaskGroup != nullptr &&
             "Should have at least one taskgroup!");

      OmpHappensAfter(Data->TaskGroup->GetPtr());

      Data->InBarrier = false;
      thread_local_clock->exitState("SyncRegionEnd", Data->isRunning);
      // Delete this allocated taskgroup, all descendent task are finished by
      // now.
      Taskgroup *Parent = Data->TaskGroup->Parent;
      Data->TaskGroup->Delete();
      Data->TaskGroup = Parent;
      break;
    }

    case cpt_sync_region_reduction:
      // Should not occur according to OpenMP 5.1
      // Tested in OMPT tests
      break;
    }
    break;
  }
#endif
}

static void ompt_tsan_sync_region_wait(ompt_sync_region_t kind,
                                       ompt_scope_endpoint_t endpoint,
                                       ompt_data_t *parallel_data,
                                       ompt_data_t *task_data,
                                       const void *codeptr_ra) {
#ifndef EMPTYCB
  overheadTimer ovT{2};
  TaskData *Data = ToTaskData(task_data);
  if (kind == ompt_sync_region_taskgroup) {
    switch (endpoint) {
    case ompt_scope_begin:
    case ompt_scope_beginend:
      if (analysis_flags->running)
        omptThreadCount->syncRegionBegin++;
      DCHECK_NOT(Data->InCreate);
      Data->InBarrier = true;
      thread_local_clock->enterState(STATE_OMP, "SyncRegionBegin");
      if (endpoint == ompt_scope_begin)
        break;
      KMP_FALLTHROUGH();
    case ompt_scope_end:
      if (analysis_flags->running)
        omptThreadCount->syncRegionEnd++;

      break;
    }
  }
#endif
}

static int ompt_tsan_control_tool(uint64_t command, uint64_t modifier,
                                  void *arg, const void *codeptr_ra) {
  if (command == omp_control_tool_start) {
    startTool();
  } else if (command == omp_control_tool_pause) {
    return 1;
  } else if (command == omp_control_tool_flush) {
    return 1;
  } else if (command == omp_control_tool_end) {
    stopTool();
  }
  return 0;
}

/// OMPT event callbacks for handling tasks.

static void ompt_tsan_task_create(
    ompt_data_t *parent_task_data,    /* id of parent task            */
    const ompt_frame_t *parent_frame, /* frame data for parent task   */
    ompt_data_t *new_task_data,       /* id of created task           */
    int type, int has_dependences,
    const void *codeptr_ra) /* pointer to outlined function */
{
  if (analysis_flags->running)
    omptThreadCount->taskCreate++;
#ifndef EMPTYCB
  overheadTimer ovT{0};
  if (hasTaskProperty > ompt_set_never) {
    thread_local_clock->enterState(STATE_OMP, "TaskCreationBegin");
    ToTaskData(parent_task_data)->InCreate = true;
  }
  TaskData *Data;
  DCHECK(new_task_data->ptr == NULL &&
         "Task data should be initialized to NULL");
  if (type & cpt_task_initial) {
    ompt_data_t *parallel_data;
    int team_size = 1;
    critical_ompt_get_parallel_info(0, &parallel_data, &team_size);
    ParallelData *PData = ParallelData::New(nullptr);
    parallel_data->ptr = PData;

    Data = TaskData::New(PData, type);
    new_task_data->ptr = Data;
  } else if (type & cpt_task_explicit || type & cpt_task_target ||
             type & cpt_task_taskwait) {
    Data = TaskData::New(ToTaskData(parent_task_data), type, codeptr_ra);
    new_task_data->ptr = Data;

    // Use the newly created address. We cannot use a single address from the
    // parent because that would declare wrong relationships with other
    // sibling tasks that may be created before this task is started!
    ompTimer ot{"TaskCreate"};
    OmpHappensBefore(Data->GetTaskPtr());
    ToTaskData(parent_task_data)->execution++;
  }
  if (analysis_flags->tasking && Data->isUntied())
    fprintf(stderr, "Warning: Task-level analysis not yet supported for "
                    "untied tasks\n");
#endif
}

static void
ompt_tsan_task_creation(ompt_scope_endpoint_t endpoint,
                        ompt_data_t *parent_task_data, /* id of parent task */
                        ompt_data_t *new_task_data) /* id of created task    */
{
#ifndef EMPTYCB
  overheadTimer ovT{0};
  switch (endpoint) {
  case ompt_scope_begin:
    // runtime overhead, enter OpenMP
    thread_local_clock->enterState(STATE_OMP, "TaskCreationBegin");
    break;
  case ompt_scope_end:
    // runtime overhead, exit OpenMP
    thread_local_clock->exitState("TaskCreationEnd");
    break;
  case ompt_scope_beginend:
    // Should not occur according to OpenMP 5.1
    // Tested in OMPT tests
    break;
  }
#endif
}

static void ompt_tsan_task_property(ompt_data_t *task_data,
                                    cpt_x_task_property_t kind,
                                    void *property) {
#ifndef EMPTYCB
  switch (kind) {
  case cpt_x_task_property_created: {
    overheadTimer ovT{0};
    // ToTaskData(property) is the TaskData for the created task
    thread_local_clock->exitState("TaskCreation");
    DCHECK(ToTaskData(task_data)->InCreate);
    ToTaskData(task_data)->InCreate = false;
    break;
  }
  case cpt_x_task_property_priority: {
    cpt_x_task_property_priority_t *taskPriority =
        (cpt_x_task_property_priority_t *)(property);
    ToTaskData(task_data)->priority = taskPriority->priority;
    break;
  }
  case cpt_x_task_property_name: {
    cpt_x_task_property_name_t *taskName =
        (cpt_x_task_property_name_t *)(property);
    std::string name = taskName->name;
    ToTaskData(task_data)->name = name;
    break;
  }
  default:
    break;
  }
#endif
}

static void freeTask(TaskData *task) {
  while (task != nullptr && --task->RefCount == 0) {
    TaskData *Parent = task->Parent;
    task->Delete();
    task = Parent;
  }
}

static void releaseDependencies(TaskData *task) {
  for (unsigned i = 0; i < task->DependencyCount; i++) {
    task->Dependencies[i].AnnotateEnd();
  }
}

static void acquireDependencies(TaskData *task) {
  for (unsigned i = 0; i < task->DependencyCount; i++) {
    task->Dependencies[i].AnnotateBegin();
  }
}

static void completeTask(TaskData *FromTask,
                         ompt_task_status_t prior_task_status) {
  if (!FromTask)
    return;

// Task is finished, so include runtime in task runtime statistics
#ifndef EMPTYCB
  if (analysis_flags->running && omptThreadCount) {
    THREAD_CLOCK *fiber =
        reinterpret_cast<THREAD_CLOCK *>(OmpGetCurrentFiber());
    double taskTime = fiber->clocks[CLOCK_USEFUL].thread.getTime();
    omptThreadCount->totalTasks++;
    omptThreadCount->totalTaskTime += taskTime;
    omptThreadCount->minTaskTime =
        std::min(omptThreadCount->minTaskTime, taskTime);
    omptThreadCount->maxTaskTime =
        std::max(omptThreadCount->maxTaskTime, taskTime);
  }
#endif
  // Task-end happens after a possible omp_fulfill_event call
  if (FromTask->isFulfilled())
    OmpHappensAfter(FromTask->GetTaskPtr());
  // Included tasks are executed sequentially, no need to track
  // synchronization
  if (!FromTask->isUndeferrable() && !FromTask->isFinalUndeferred() ||
      prior_task_status == ompt_task_late_fulfill) {
    // Task will finish before a barrier in the surrounding parallel region
    // ...
    ParallelData *PData = FromTask->Team;
    OmpHappensBefore(
        PData->GetBarrierPtr(FromTask->ImplicitTask->BarrierIndex));

    // ... and before an eventual taskwait by the parent thread.
    OmpHappensBefore(FromTask->Parent->GetTaskwaitPtr());

    if (FromTask->TaskGroup != nullptr) {
      // This task is part of a taskgroup, so it will finish before the
      // corresponding taskgroup_end.
      OmpHappensBefore(FromTask->TaskGroup->GetPtr());
    }
  } else {
    OmpHappensBefore(FromTask->Parent->GetTaskPtr());
  }
  // release dependencies
  releaseDependencies(FromTask);
}

static void completeExecutionTask(TaskData *FromTask) {
  if (!FromTask)
    return;
  OmpHappensBefore(FromTask->Parent->GetTaskPtr());
}

static void suspendTask(TaskData *FromTask, TaskData *ToTask) {
  if (!FromTask)
    return;
  // Task may be resumed at a later point in time.
  if (FromTask->isUntied()) {
    if (FromTask->InBarrier || FromTask->InCreate) {
      thread_local_clock->enterState(STATE_OMP, "UntiedSuspend");
    } else {
      thread_local_clock->exitState("UntiedSuspend", FromTask->isRunning);
      OmpHappensBefore(FromTask->GetTaskPtr());
    }
  } else {
    thread_local_clock->enterState(STATE_OMP, "TiedSuspend");
  }
  ToTask->ImplicitTask = FromTask->ImplicitTask;
  DCHECK(ToTask->ImplicitTask != NULL &&
         "A task belongs to a team and has an implicit task on the stack");
}

static void suspendTaskEnd(TaskData *FromTask, TaskData *ToTask) {
  if (!FromTask)
    return;
  // Task may be resumed at a later point in time.
  thread_local_clock->exitState(STATE_USEFUL, STATE_OMP, "TaskEnd",
                                FromTask->isRunning);
}

static void switchTasks(TaskData *FromTask, TaskData *ToTask) {
  if (FromTask)
    FromTask->deactivate();
  if (ToTask)
    ToTask->activate();
}

static void startTask(TaskData *ToTask) {
  if (!ToTask)
    return;
  // Handle dependencies on first execution of the task
  bool startingTask = false;
  if (ToTask->execution == 0) {
    startingTask = true;
    ToTask->execution++;
    acquireDependencies(ToTask);
  }
  // 1. Task will begin execution after it has been created.
  // 2. Task will resume after it has been switched away.
  if (startingTask) {
    OmpHappensAfter(ToTask->GetTaskPtr());
    thread_local_clock->enterState(STATE_USEFUL, "TaskBegin");
    if ((analysis_flags->collect_task_counts_on_cp) &&
        (!ToTask->isImplicit()) &&
        (!ToTask->isInitial())) // TODO: check why this also works without the
                                // implicit/initial check
      TaskCountMetricHandler::getInstance()->increaseTaskCount(1);
  } else if (ToTask->isUntied()) {
    if (ToTask->InBarrier || ToTask->InCreate) {
      thread_local_clock->exitState(STATE_OMP, STATE_OMP, "TaskContinueUntied",
                                    ToTask->isRunning);
    } else {
      OmpHappensAfter(ToTask->GetTaskPtr());
      thread_local_clock->enterState(STATE_USEFUL, "TaskContinueUntied");
    }
  } else {
    if (ToTask->InBarrier || ToTask->InCreate)
      thread_local_clock->exitState(STATE_OMP, STATE_OMP, "TaskContinue",
                                    ToTask->isRunning);
    else
      thread_local_clock->exitState(STATE_OMP, STATE_USEFUL, "TaskContinue",
                                    ToTask->isRunning);
  }
}

static void ompt_tsan_task_schedule(ompt_data_t *first_task_data,
                                    ompt_task_status_t prior_task_status,
                                    ompt_data_t *second_task_data) {
#ifndef EMPTYCB
  overheadTimer ovT{1};
  if (analysis_flags->running && omptThreadCount)
    omptThreadCount->taskSchedule++;

  //
  //  The necessary action depends on prior_task_status:
  //
  //    ompt_task_early_fulfill = 5,
  //     -> first got fulfill event, second ignored
  //
  //    ompt_task_late_fulfill  = 6,
  //     -> first completed, first freed, second ignored
  //
  //    ompt_taskwait_complete = 8,
  //     -> first starts, first completes, first freed, second ignored
  //
  //    ompt_task_complete      = 1,
  //    ompt_task_cancel        = 3,
  //     -> first completed, first freed, second starts
  //
  //    ompt_taskwait_complete = 8,
  //     -> first starts, first completes, first freed, second ignored
  //
  //    ompt_task_detach        = 4,
  //    ompt_task_yield         = 2,
  //    ompt_task_switch        = 7
  //     -> first suspended, second starts
  //

  TaskData *FromTask = first_task_data ? ToTaskData(first_task_data) : nullptr;
  TaskData *ToTask = second_task_data ? ToTaskData(second_task_data) : nullptr;

  DCHECK(thread_local_clock);

  switch (prior_task_status) {
  case ompt_task_early_fulfill: {
    ompTimer ot{"EarlyFulfill"};
    OmpHappensBefore(FromTask->GetTaskPtr());
    FromTask->setFulfilled();
    return;
  }
  case ompt_task_late_fulfill: {
    ompTimer ot{"LateFulfill"};
    OmpHappensAfter(FromTask->GetTaskPtr());
    completeTask(FromTask, prior_task_status);
    freeTask(FromTask);
    return;
  }
  case ompt_taskwait_complete: {
    ompTimer ot{"TaskwaitNowaitDepend"};
    acquireDependencies(FromTask);
    freeTask(FromTask);
    return;
  }
  case ompt_task_complete:
    suspendTaskEnd(FromTask, ToTask);
    completeTask(FromTask, prior_task_status);
    switchTasks(FromTask, ToTask);
    if (FromTask && FromTask->isFinalUndeferred())
      OmpHappensAfter(FromTask->Parent->GetTaskPtr());
    freeTask(FromTask);
    startTask(ToTask);
    return;
  case ompt_task_cancel:
    suspendTaskEnd(FromTask, ToTask);
    completeTask(FromTask, prior_task_status);
    switchTasks(FromTask, ToTask);
    freeTask(FromTask);
    startTask(ToTask);
    return;
  case ompt_task_detach:
    suspendTaskEnd(FromTask, ToTask);
    completeExecutionTask(FromTask);
    switchTasks(FromTask, ToTask);
    startTask(ToTask);
    return;
  case ompt_task_yield:
    suspendTask(FromTask, ToTask);
    switchTasks(FromTask, ToTask);
    startTask(ToTask);
    return;
  case ompt_task_switch:
    suspendTask(FromTask, ToTask);
    switchTasks(FromTask, ToTask);
    startTask(ToTask);
    return;
  }
#endif
}

static void ompt_tsan_dependences(ompt_data_t *task_data,
                                  const ompt_dependence_t *deps, int ndeps) {
#ifndef EMPTYCB
  overheadTimer ovT{3};
  if (ndeps > 0) {
    ompTimer ot{"TaskDepend"};
    // Copy the data to use it in task_switch and task_end.
    TaskData *Data = ToTaskData(task_data);
    if (!Data->Parent->DependencyMap)
      Data->Parent->DependencyMap =
          new CompactHashMap<void *, DependencyData *>();
    Data->Dependencies =
        (TaskDependency *)malloc(sizeof(TaskDependency) * ndeps);
    Data->DependencyCount = ndeps;
    for (int i = 0, d = 0; i < ndeps; i++, d++) {
      auto ret = Data->Parent->DependencyMap->Insert(
          Pair<void *, DependencyData *>({deps[i].variable.ptr, nullptr}));
      if (ret.second) {
        ret.first->second = DependencyData::New();
      }
      new ((void *)(Data->Dependencies + d))
          TaskDependency(ret.first->second, deps[i].dependence_type);
    }

    // This callback is executed before this task is first started.
    OmpHappensBefore(Data->GetTaskPtr());
  }
#endif
}

static void ompt_tsan_mutex_acquire(ompt_mutex_t kind, unsigned int hint,
                                    unsigned int impl, ompt_wait_id_t wait_id,
                                    const void *codeptr_ra) {
#ifndef EMPTYCB
  if (analysis_flags->running)
    omptThreadCount->mutexAcquire++;
  thread_local_clock->enterState(STATE_OMP, "MutexAcquire");
#endif
}

/// OMPT event callbacks for handling locking.
static void ompt_tsan_mutex_acquired(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                                     const void *codeptr_ra) {
#ifndef EMPTYCB
  //    ompTimer ot{};
  // Acquire our own lock to make sure that
  // 1. the previous release has finished.
  // 2. the next acquire doesn't start before we have finished our release.
  LocksMutex.lock();
  auto InsertPair = Locks.Insert(
      Pair<ompt_wait_id_t, Pair<std::mutex, sync_clock_composite> *>(
          {wait_id, nullptr}));
  if (InsertPair.second) { // true on successfull insertion
    InsertPair.first->second = new Pair<std::mutex, sync_clock_composite>();
  }
  // InsertPair iterator can get invalidated on rehash
  auto LockIdPair = InsertPair.first->second;
  LocksMutex.unlock();
  LockIdPair->first.lock();
  OmpHappensAfter(&LockIdPair->second);
  thread_local_clock->exitState("MutexAcquired");
#endif
}

static void ompt_tsan_mutex_released(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                                     const void *codeptr_ra) {
#ifndef EMPTYCB
  ompTimer ot{"MutexRelease"};
  LocksMutex.lock();
  auto &Lock = Locks[wait_id];
  LocksMutex.unlock();
  OmpHappensBefore(&Lock->second);

  Lock->first.unlock();
#endif
}

// callback, signature, variable to store result, selector, required support
// level
#define SET_OPTIONAL_X_SELECTIVE_CALLBACK_T(event, type, select, result,       \
                                            level)                             \
  do {                                                                         \
    result = ompt_set_never;                                                   \
    if (cpt_x_set_selective_callback) {                                        \
      cpt_x_callback_##type##_t tsan_##event = &ompt_tsan_##event;             \
      result = cpt_x_set_selective_callback(                                   \
          select, (ompt_callbacks_t)cpt_x_callback_##event,                    \
          (ompt_callback_t)tsan_##event);                                      \
    }                                                                          \
    if (result < level)                                                        \
      printf("Registered callback '" #event "' is not supported at " #level    \
             " (%i)\n",                                                        \
             result);                                                          \
  } while (0)

// callback , signature , variable to store result , required support level
#define SET_OPTIONAL_CALLBACK_T(event, type, result, level)                    \
  do {                                                                         \
    ompt_callback_##type##_t tsan_##event = &ompt_tsan_##event;                \
    result = ompt_set_callback(ompt_callback_##event,                          \
                               (ompt_callback_t)tsan_##event);                 \
    if (result < level)                                                        \
      printf("Registered callback '" #event "' is not supported at " #level    \
             " (%i)\n",                                                        \
             result);                                                          \
  } while (0)

#define SET_CALLBACK_T(event, type)                                            \
  do {                                                                         \
    int res;                                                                   \
    SET_OPTIONAL_CALLBACK_T(event, type, res, ompt_set_always);                \
  } while (0)

#define SET_CALLBACK(event) SET_CALLBACK_T(event, event)

#define findTsanFunction(f, fSig)                                              \
  do {                                                                         \
    if (NULL == (f = fSig dlsym(RTLD_DEFAULT, #f)))                            \
      printf("Unable to find TSan function " #f ".\n");                        \
  } while (0)

#define findTsanFunctionSilent(f, fSig) f = fSig dlsym(RTLD_DEFAULT, #f)
#define findTsanFunctionName(f, name, fSig) f = fSig dlsym(RTLD_DEFAULT, #name)

static int ompt_tsan_initialize(ompt_function_lookup_t lookup, int device_num,
                                ompt_data_t *tool_data) {

  ompt_set_callback_t ompt_set_callback =
      (ompt_set_callback_t)lookup("ompt_set_callback");
  if (ompt_set_callback == NULL) {
    fprintf(stderr, "Could not set callback, exiting...\n");
    std::exit(1);
  }
  cpt_x_set_selective_callback_t cpt_x_set_selective_callback =
      (cpt_x_set_selective_callback_t)lookup("ompt_x_set_selective_callback");
  critical_ompt_get_parallel_info =
      (ompt_get_parallel_info_t)lookup("ompt_get_parallel_info");
  critical_ompt_finalize_tool =
      (ompt_finalize_tool_t)lookup("ompt_finalize_tool");

  if (critical_ompt_get_parallel_info == NULL) {
    fprintf(stderr, "Could not get inquiry function 'ompt_get_parallel_info', "
                    "exiting...\n");
    exit(1);
  }

  SET_CALLBACK(thread_begin);
  SET_CALLBACK(thread_end);
  SET_CALLBACK(parallel_begin);
  SET_CALLBACK(implicit_task);
  SET_CALLBACK(sync_region);
  SET_CALLBACK_T(sync_region_wait, sync_region);
  SET_CALLBACK(parallel_end);
  SET_CALLBACK(control_tool);

  SET_CALLBACK(task_create);
  // SET_OPTIONAL_CALLBACK_T(task_creation, task_creation, hasTaskCreation,
  // ompt_set_never);
  SET_OPTIONAL_X_SELECTIVE_CALLBACK_T(task_property, task_property,
                                      cpt_x_task_property_created |
                                          cpt_x_task_property_priority |
                                          cpt_x_task_property_name,
                                      hasTaskProperty, ompt_set_never);
  if (hasTaskProperty > ompt_set_never)
    printf("We have the task priority callback\n");
  SET_CALLBACK(task_schedule);
  SET_CALLBACK(dependences);

  SET_CALLBACK(mutex_acquire);
  SET_CALLBACK_T(mutex_acquired, mutex);
  SET_CALLBACK_T(mutex_released, mutex);

  if (!thread_local_clock) {
    OmpFiberPoolInit();
    thread_local_clock =
        THREAD_CLOCK::New(my_next_id(), 0, ompt_thread_initial, true);
    // printf("Thread %i: Created thread_local_clock %p in initialize with fiber
    // %p\n", omp_get_thread_num(), thread_local_clock,
    // thread_local_clock->fiber);
  }
  if (analysis_flags->collect_task_type_share)
    DepMetricOrganizer::getInstance()->registerDoubleMetricHandler(
        TaskTypeMetricHandler::getInstance());
  if (strcmp(analysis_flags->task_prio_shares, "") > 0) {
    DepMetricOrganizer::getInstance()->registerDoubleMetricHandler(
        TaskPrioMetricHandler::getInstance());
    TaskPrioMetricHandler::getInstance()->addTaskPrios();
  }
  if (strcmp(analysis_flags->task_name_shares, "") > 0) {
    DepMetricOrganizer::getInstance()->registerDoubleMetricHandler(
        TaskNameMetricHandler::getInstance());
    TaskNameMetricHandler::getInstance()->addTaskNames();
  }
  if (analysis_flags->collect_task_counts_on_cp)
    DepMetricOrganizer::getInstance()->registerIntMetricHandler(
        TaskCountMetricHandler::getInstance());
  startTool(false);
  return 1; // success
}

static void ompt_tsan_finalize(ompt_data_t *tool_data) {
  if (!useMpi) {
    if (analysis_flags->verbose)
      fprintf(analysis_flags->output, "Max Useful Computation -- %2.5lf\n",
              crit_path_useful_time);
    finishMeasurement();
  }
  if (analysis_flags->print_max_rss) {
    struct rusage end;
    getrusage(RUSAGE_SELF, &end);
    fprintf(analysis_flags->output, "MAX RSS[KBytes] during execution: %ld\n",
            end.ru_maxrss);
  }
#ifndef EMPTYCB
  OmpFiberPoolFini();
#endif
}

extern "C" __attribute__((visibility("default"))) ompt_start_tool_result_t *
ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
  InitializeCptFlags();
  if (!analysis_flags->enabled) {
    if (analysis_flags->verbose)
      fprintf(stderr, "Tool disabled, stopping operation\n");
    return NULL;
  }
  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Starting critPathAnalysis OMPT tool\n");

#ifdef USE_ERRHANDLER
  init_signalhandlers();
#endif

  pagesize = getpagesize();

  static ompt_start_tool_result_t ompt_start_tool_result = {
      &ompt_tsan_initialize, &ompt_tsan_finalize, {0}};
  if (!thread_clocks)
    thread_clocks = new Vector<THREAD_CLOCK *>{};
  if (!thread_counts)
    thread_counts = new Vector<omptCounts<double> *>{};

  // The OMPT start-up code uses dlopen with RTLD_LAZY. Therefore, we cannot
  // rely on dlopen to fail if TSan is missing, but would get a runtime error
  // for the first TSan call. We use RunningOnValgrind to detect whether
  // an implementation of the Annotation interface is available in the
  // execution or disable the tool (by returning NULL).

  if (analysis_flags->verbose && !analysis_flags->stopped)
    fprintf(analysis_flags->output,
            "Tool enabled, collecting critical path.\n");
  if (analysis_flags->verbose && analysis_flags->stopped)
    fprintf(analysis_flags->output, "Tool enabled, waiting to get started.\n");
  return &ompt_start_tool_result;
}
