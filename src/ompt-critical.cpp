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
#include <mutex>
#include <omp-tools.h>
#include <omp.h>

#ifdef USE_CUDA
#include "cuda-tracking.h"
#endif

// For debugging
static const char *ompt_scope_endpoint_t_values[] = {
    "", "ompt_scope_begin", "ompt_scope_end", "ompt_scope_beginend"};

static const char *ompt_target_t_values[] = {"",
                                             "ompt_target",
                                             "ompt_target_enter_data",
                                             "ompt_target_exit_data",
                                             "ompt_target_update",
                                             "",
                                             "",
                                             "",
                                             "",
                                             "",
                                             "ompt_target_nowait",
                                             "ompt_target_enter_data_nowait",
                                             "ompt_target_exit_data_nowait",
                                             "ompt_target_update_nowait"};

// Define attribute that indicates that the fall through from the previous
// case label is intentional and should not be diagnosed by a compiler
//   Code from libcxx/include/__config
// Use a function like macro to imply that it must be followed by a semicolon
#if __cplusplus > 201402L && __has_cpp_attribute(fallthrough)
#define KMP_FALLTHROUGH() [[fallthrough]]
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

// static TargetOmptCounts;
static __thread omptCounts *omptThreadCount;

// thread_local KernelData *thread_local_taskdata = nullptr;

struct ompTimer {
  const char *loc;
  ompTimer(const char *loc = NULL) : loc(loc) {
    if (thread_local_clock == nullptr)
      thread_local_clock = new THREAD_CLOCK(my_next_id(), 0);
    thread_local_clock->enterState(STATE_OMP, loc);
  }
  ~ompTimer() { thread_local_clock->exitState(loc); }
};

int __cpt::pagesize{4096};

struct DependencyData;
typedef DataPool<DependencyData> DependencyDataPool;
template <>
__thread DependencyDataPool *DependencyDataPool::ThreadDataPool = nullptr;

/// Data structure to store additional information for task dependency.
struct DependencyData final : DataPoolEntry<DependencyData> {
  ompt_tsan_clockid in;
  ompt_tsan_clockid out;
  ompt_tsan_clockid inoutset;
  ompt_tsan_clockid *GetInPtr() { return &in; }
  ompt_tsan_clockid *GetOutPtr() { return &out; }
  ompt_tsan_clockid *GetInoutsetPtr() { return &inoutset; }

  void Reset() {}

  static DependencyData *New() {
    if (!DependencyDataPool::ThreadDataPool)
      DependencyDataPool::ThreadDataPool = new DependencyDataPool();
    return DataPoolEntry<DependencyData>::New();
  }

  DependencyData(DataPool<DependencyData> *dp)
      : DataPoolEntry<DependencyData>(dp) {}
};

struct TaskDependency {
  ompt_tsan_clockid *inPtr;
  ompt_tsan_clockid *outPtr;
  ompt_tsan_clockid *inoutsetPtr;
  ompt_dependence_type_t type;
  TaskDependency(DependencyData *depData, ompt_dependence_type_t type)
      : inPtr(depData->GetInPtr()), outPtr(depData->GetOutPtr()),
        inoutsetPtr(depData->GetInoutsetPtr()), type(type) {}
  void AnnotateBegin(THREAD_CLOCK *tc = thread_local_clock) {
    if (type == ompt_dependence_type_out ||
        type == ompt_dependence_type_inout ||
        type == ompt_dependence_type_mutexinoutset) {
      OmpHappensAfter(inPtr, tc);
      OmpHappensAfter(outPtr, tc);
      OmpHappensAfter(inoutsetPtr, tc);
    } else if (type == ompt_dependence_type_in) {
      OmpHappensAfter(outPtr, tc);
      OmpHappensAfter(inoutsetPtr, tc);
    } else if (type == ompt_dependence_type_inoutset) {
      OmpHappensAfter(inPtr, tc);
      OmpHappensAfter(outPtr, tc);
    }
  }
  void AnnotateEnd(THREAD_CLOCK *tc = thread_local_clock) {
    if (type == ompt_dependence_type_out ||
        type == ompt_dependence_type_inout ||
        type == ompt_dependence_type_mutexinoutset) {
      OmpHappensBefore(outPtr, tc);
    } else if (type == ompt_dependence_type_in) {
      OmpHappensBefore(inPtr, tc);
    } else if (type == ompt_dependence_type_inoutset) {
      OmpHappensBefore(inoutsetPtr, tc);
    }
  }
  void GpuAnnotateBegin(THREAD_CLOCK *tc = thread_local_clock) {
    if (type == ompt_dependence_type_out ||
        type == ompt_dependence_type_inout ||
        type == ompt_dependence_type_mutexinoutset) {
      GpuHappensAfter(inPtr, tc);
      GpuHappensAfter(outPtr, tc);
      GpuHappensAfter(inoutsetPtr, tc);
    } else if (type == ompt_dependence_type_in) {
      GpuHappensAfter(outPtr, tc);
      GpuHappensAfter(inoutsetPtr, tc);
    } else if (type == ompt_dependence_type_inoutset) {
      GpuHappensAfter(inPtr, tc);
      GpuHappensAfter(outPtr, tc);
    }
  }
  void GpuAnnotateEnd(THREAD_CLOCK *tc = thread_local_clock) {
    if (type == ompt_dependence_type_out ||
        type == ompt_dependence_type_inout ||
        type == ompt_dependence_type_mutexinoutset) {
      GpuHappensBefore(outPtr, tc);
    } else if (type == ompt_dependence_type_in) {
      GpuHappensBefore(inPtr, tc);
    } else if (type == ompt_dependence_type_inoutset) {
      GpuHappensBefore(inoutsetPtr, tc);
    }
  }
};

struct ParallelData;
typedef DataPool<ParallelData> ParallelDataPool;
template <>
__thread ParallelDataPool *ParallelDataPool::ThreadDataPool = nullptr;

/// Data structure to store additional information for parallel regions.
struct ParallelData final : DataPoolEntry<ParallelData> {

  // Parallel fork is just another barrier, use Barrier[1]

  /// Two addresses for relationships with barriers.
  ompt_tsan_clockid Barrier[3];

  const void *codePtr;

  ompt_tsan_clockid *GetParallelPtr() { return &(Barrier[1]); }

  ompt_tsan_clockid *GetBarrierPtr(unsigned Index) { return &(Barrier[Index]); }

  ParallelData *Init(const void *codeptr) {
    codePtr = codeptr;
    return this;
  }

  void Reset() {}

  static ParallelData *New(const void *codeptr) {
    if (!ParallelDataPool::ThreadDataPool)
      ParallelDataPool::ThreadDataPool = new ParallelDataPool();
    return DataPoolEntry<ParallelData>::New()->Init(codeptr);
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
  ompt_tsan_clockid Ptr;

  /// Reference to the parent taskgroup.
  Taskgroup *Parent;

  ompt_tsan_clockid *GetPtr() { return &Ptr; }

  Taskgroup *Init(Taskgroup *parent) {
    Parent = parent;
    return this;
  }

  void Reset() {}

  static Taskgroup *New(Taskgroup *Parent) {
    if (!TaskgroupPool::ThreadDataPool)
      TaskgroupPool::ThreadDataPool = new TaskgroupPool();
    return DataPoolEntry<Taskgroup>::New()->Init(Parent);
  }

  Taskgroup(DataPool<Taskgroup> *dp) : DataPoolEntry<Taskgroup>(dp) {}
};

struct TaskData;
typedef DataPool<TaskData> TaskDataPool;
template <> __thread TaskDataPool *TaskDataPool::ThreadDataPool = nullptr;

enum CptTaskFlag { CptTaskFulfilled = 0x00010000 };

/// Data structure to store additional information for tasks.
struct TaskData final : DataPoolEntry<TaskData>, KernelData {
  /// Its address is used for relationships of this task.
  ompt_tsan_clockid Task{0};

  /// Child tasks use its address to declare a relationship to a taskwait in
  /// this task.
  ompt_tsan_clockid Taskwait{0};

  /// Whether this task was started while analysis_flags->running == true
  bool isRunning{false};

  /// Whether this task is currently executing a barrier.
  bool InBarrier{false};

  /// Whether this task is an included task.
  int TaskType{0};

  int ThreadNum{-1};

  /// count execution phase
  int execution{0};

  /// Index of which barrier to use next.
  char BarrierIndex{0};

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

  /// TODO THREAD_CLOCK for target tasks - moved to target_data
  THREAD_CLOCK *TargetThreadClock{nullptr};

  double StartTime{0};

  /// Device id
  int device_num;
#ifdef DEBUG
  int freed{0};
#endif

  bool isIncluded() { return TaskType & ompt_task_undeferred; }
  bool isUntied() { return TaskType & ompt_task_untied; }
  bool isFinal() { return TaskType & ompt_task_final; }
  bool isMergable() { return TaskType & ompt_task_mergeable; }
  bool isMerged() { return TaskType & ompt_task_merged; }

  bool isExplicit() { return TaskType & ompt_task_explicit; }
  bool isImplicit() { return TaskType & ompt_task_implicit; }
  bool isInitial() { return TaskType & ompt_task_initial; }
  bool isTarget() { return TaskType & ompt_task_target; }

  bool isFulfilled() { return TaskType & CptTaskFulfilled; }
  void setFulfilled() { TaskType |= CptTaskFulfilled; }

  ompt_tsan_clockid *GetTaskPtr() { return &Task; }

  ompt_tsan_clockid *GetTaskwaitPtr() { return &Taskwait; }

  TaskData *Init(TaskData *parent, int taskType, THREAD_CLOCK *threadClock,
                 int deviceNum) {
    isRunning = analysis_flags->running;
    TaskType = taskType;
    Parent = parent;
    Team = Parent->Team;
    ImplicitTask = Parent->ImplicitTask;
    if (Parent != nullptr) {
      Parent->RefCount++;
      // Copy over pointer to taskgroup. This task may set up its own stack
      // but for now belongs to its parent's taskgroup.
      TaskGroup = Parent->TaskGroup;
    }
    if (threadClock) {
      TargetThreadClock = threadClock;
    }
    device_num = deviceNum;
    return this;
  }

  TaskData *Init(ParallelData *team, int taskType) {
    isRunning = analysis_flags->running;
    TaskType = taskType;
    execution = 1;
    ImplicitTask = this;
    Team = team;
    return this;
  }

  TaskData *Init(ParallelData *team, int threadNum, int taskType) {
    isRunning = analysis_flags->running;
    TaskType = taskType;
    execution = 1;
    ImplicitTask = this;
    Team = team;
    ThreadNum = threadNum;
    return this;
  }

  void freeTask() {
    TaskData *task = this;
    while (task != nullptr && --task->RefCount == 0) {
      TaskData *Parent = task->Parent;
      task->Delete();
      task = Parent;
    }
  }

  void releaseDependencies(THREAD_CLOCK *tc_arg) {
    TaskData *task = this;
    for (unsigned i = 0; i < task->DependencyCount; i++) {
      task->Dependencies[i].AnnotateEnd(tc_arg);
    }
  }

  void acquireDependencies(THREAD_CLOCK *tc_arg) {
    TaskData *task = this;
    for (unsigned i = 0; i < task->DependencyCount; i++) {
      task->Dependencies[i].AnnotateBegin(tc_arg);
    }
  }

  void gpuReleaseDependencies(THREAD_CLOCK *tc_arg) {
    TaskData *task = this;
    for (unsigned i = 0; i < task->DependencyCount; i++) {
      task->Dependencies[i].GpuAnnotateEnd(tc_arg);
    }
  }

  void gpuAcquireDependencies(THREAD_CLOCK *tc_arg) {
    TaskData *task = this;
    for (unsigned i = 0; i < task->DependencyCount; i++) {
      task->Dependencies[i].GpuAnnotateBegin(tc_arg);
    }
  }

  void finish(double dur, uint32_t streamId, uint32_t deviceId = 0) {
    // finish simulates starting, execution, and completion of the kernel
    // finish also frees the KernelData
    auto streamClock = getOrCreateStreamClock(streamId, deviceId);

    gpuAcquireDependencies(streamClock);

    GpuHappensAfter(GetTaskPtr(), streamClock);
    // TODO: Fix clock context
    GpuExecKernel(streamClock, dur);

    // if (!isMerged()) {
    if (Team && Parent) {
      // sync "task local" tlc with regular tlc
      ParallelData *PData = Team;
      GpuHappensBefore(PData->GetBarrierPtr(ImplicitTask->BarrierIndex),
                       streamClock);

      // ... and before an eventual taskwait by the parent thread.
      GpuHappensBefore(Parent->GetTaskwaitPtr(), streamClock);
    }

    if (TaskGroup != nullptr) {
      // This task is part of a taskgroup, so it will finish before the
      // corresponding taskgroup_end.
      GpuHappensBefore(TaskGroup->GetPtr(), streamClock);
    }

    gpuReleaseDependencies(streamClock);
    freeTask();
    // } else {
    //   GpuHappensBefore(GetTaskPtr(), streamClock);
    // }
  }

  void Reset() {
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
    StartTime = 0;
    TargetThreadClock = nullptr;
#ifdef DEBUG
    freed = 0;
#endif
  }
  // explicit task
  static TaskData *New(TaskData *parent, int taskType,
                       THREAD_CLOCK *threadClock = nullptr,
                       int deviceNum = -1) {
    if (!TaskDataPool::ThreadDataPool)
      TaskDataPool::ThreadDataPool = new TaskDataPool();
    return DataPoolEntry<TaskData>::New()->Init(parent, taskType, threadClock,
                                                deviceNum);
  }

  // initial task from task_create
  static TaskData *New(ParallelData *team, int taskType) {
    if (!TaskDataPool::ThreadDataPool)
      TaskDataPool::ThreadDataPool = new TaskDataPool();
    return DataPoolEntry<TaskData>::New()->Init(team, taskType);
  }
  // implicit/initial task from implicit_task_begin
  static TaskData *New(ParallelData *team, int threadNum, int taskType) {
    if (!TaskDataPool::ThreadDataPool)
      TaskDataPool::ThreadDataPool = new TaskDataPool();
    return DataPoolEntry<TaskData>::New()->Init(team, threadNum, taskType);
  }

  TaskData(DataPool<TaskData> *dp) : DataPoolEntry<TaskData>(dp) {}
};

static inline TaskData *ToTaskData(ompt_data_t *task_data) {
  if (!task_data)
    return nullptr;
  return reinterpret_cast<TaskData *>(task_data->ptr);
}

// TODO for detached target tasks whose pseudo tcs we want to access
static inline THREAD_CLOCK *ToThreadClock(ompt_data_t *target_data) {
  return reinterpret_cast<THREAD_CLOCK *>(target_data->ptr);
}

/*
 * Threads
 */

/// Store a mutex for each wait_id to resolve race condition with callbacks.
CompactHashMap<ompt_wait_id_t, Pair<std::mutex, ompt_tsan_clockid> *> Locks;
std::mutex LocksMutex;
std::mutex omptcmutex;
std::mutex tgtcmutex;

static void ompt_tsan_thread_begin(ompt_thread_t thread_type,
                                   ompt_data_t *thread_data) {
  ParallelDataPool::ThreadDataPool = new ParallelDataPool;
  TaskgroupPool::ThreadDataPool = new TaskgroupPool;
  TaskDataPool::ThreadDataPool = new TaskDataPool;
  DependencyDataPool::ThreadDataPool = new DependencyDataPool;

  omptThreadCount = new omptCounts();
  // Worker threads are identified as openmp thread when they execute an
  // implicit task
  if (!thread_local_clock)
    thread_local_clock = new THREAD_CLOCK(my_next_id(), 0);
  thread_data->ptr = thread_local_clock;
  {
    const std::lock_guard<std::mutex> lock(omptcmutex);
    omp_thread_clocks->push_back(thread_local_clock);
    omp_thread_counts->push_back(omptThreadCount);
  }
  if (thread_type == ompt_thread_initial)
    thread_local_clock->enterState(STATE_OMP, __func__);
  else
    thread_local_clock->enterState(startProgrammTime, STATE_OMP, __func__);
}

static void ompt_tsan_thread_end(ompt_data_t *thread_data) {

  delete ParallelDataPool::ThreadDataPool;
  delete TaskgroupPool::ThreadDataPool;
  delete TaskDataPool::ThreadDataPool;
  delete DependencyDataPool::ThreadDataPool;
}

/// OMPT event callbacks for handling parallel regions.

static void ompt_tsan_parallel_begin(ompt_data_t *parent_task_data,
                                     const ompt_frame_t *parent_task_frame,
                                     ompt_data_t *parallel_data,
                                     uint32_t requested_team_size, int flag,
                                     const void *codeptr_ra) {
  ParallelData *Data = ParallelData::New(codeptr_ra);
  parallel_data->ptr = Data;

  // end of computation
  thread_local_clock->enterState(STATE_OMP, __func__);
  OmpHappensBefore(Data->GetParallelPtr());
}

static void ompt_tsan_parallel_end(ompt_data_t *parallel_data,
                                   ompt_data_t *task_data, int flag,
                                   const void *codeptr_ra) {
  ParallelData *Data = ToParallelData(parallel_data);
  thread_local_clock->exitState(__func__);

  Data->Delete();
}

static void ompt_tsan_implicit_task(ompt_scope_endpoint_t endpoint,
                                    ompt_data_t *parallel_data,
                                    ompt_data_t *task_data,
                                    unsigned int team_size,
                                    unsigned int thread_num, int type) {
  switch (endpoint) {
  case ompt_scope_begin:
    if (analysis_flags->running)
      omptThreadCount->implTaskBegin++;
    if (type & ompt_task_initial) {
      parallel_data->ptr = ParallelData::New(nullptr);
    } else {
      // In case of reusing OMP threads from a parallel region
      // created when the tool was not yet running
      if (thread_local_clock->GetState() == STATE_INIT) {
        thread_local_clock->enterState(STATE_OMP, "ImplicitTaskBegin");
      }
      OmpHappensAfter(ToParallelData(parallel_data)->GetParallelPtr());
    }
    task_data->ptr =
        TaskData::New(ToParallelData(parallel_data), thread_num, type);
    thread_local_clock->enterState(STATE_USEFUL, "ImplicitTaskBegin");
    break;
  case ompt_scope_end: {
    if (analysis_flags->running)
      omptThreadCount->implTaskEnd++;
    // end of the task, useful computation stop
    TaskData *Data = ToTaskData(task_data);
#ifdef DEBUG
    DCHECK(Data->freed == 0 && "Implicit task end should only be called once!");
    Data->freed = 1;
#endif
    thread_local_clock->exitState("ImplicitTaskEnd", Data->isRunning);
    DCHECK(Data->RefCount == 1 &&
           "All tasks should have finished at the implicit barrier!");
    Data->Delete();
    if (type & ompt_task_initial) {
      ToParallelData(parallel_data)->Delete();
    }
    break;
  }
  case ompt_scope_beginend:
    // Should not occur according to OpenMP 5.1
    // Tested in OMPT tests
    break;
  }
}

static void ompt_tsan_sync_region(ompt_sync_region_t kind,
                                  ompt_scope_endpoint_t endpoint,
                                  ompt_data_t *parallel_data,
                                  ompt_data_t *task_data,
                                  const void *codeptr_ra) {
  TaskData *Data = ToTaskData(task_data);
  switch (endpoint) {
  case ompt_scope_begin:
  case ompt_scope_beginend:
    if (analysis_flags->running)
      omptThreadCount->syncRegionBegin++;
    // runtime overhead, stop useful
    switch (kind) {
    case ompt_sync_region_barrier_implementation:
    case ompt_sync_region_barrier_implicit:
    case ompt_sync_region_barrier_explicit:
    case ompt_sync_region_barrier_implicit_parallel:
    case ompt_sync_region_barrier_implicit_workshare:
    case ompt_sync_region_barrier_teams:
    case ompt_sync_region_barrier: {
      Data->InBarrier = true;
      thread_local_clock->enterState(STATE_OMP, "SyncRegionBegin");
      char BarrierIndex = Data->BarrierIndex;
      OmpHappensBefore(Data->Team->GetBarrierPtr(BarrierIndex));
      if (Data->ThreadNum == 0)
        OmpClockReset(Data->Team->GetBarrierPtr((BarrierIndex + 1) % 3));
      break;
    }

    case ompt_sync_region_taskwait: {
      Data->InBarrier = true;
      thread_local_clock->enterState(STATE_OMP, "SyncRegionBegin");
      break;
    }

    case ompt_sync_region_taskgroup:
      Data->TaskGroup = Taskgroup::New(Data->TaskGroup);
      break;

    case ompt_sync_region_reduction:
      // should never be reached
      break;
    }
    if (endpoint == ompt_scope_begin)
      break;
    KMP_FALLTHROUGH();
  case ompt_scope_end:
    if (analysis_flags->running)
      omptThreadCount->syncRegionEnd++;
    switch (kind) {
    case ompt_sync_region_barrier_implementation:
    case ompt_sync_region_barrier_implicit:
    case ompt_sync_region_barrier_explicit:
    case ompt_sync_region_barrier_implicit_parallel:
    case ompt_sync_region_barrier_implicit_workshare:
    case ompt_sync_region_barrier_teams:
    case ompt_sync_region_barrier: {
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

    case ompt_sync_region_taskwait: {
      if (Data->execution > 1)
        OmpHappensAfter(Data->GetTaskwaitPtr());
      Data->InBarrier = false;
      thread_local_clock->exitState("SyncRegionEnd", Data->isRunning);
      break;
    }

    case ompt_sync_region_taskgroup: {
      DCHECK(Data->TaskGroup != nullptr &&
             "Should have at least one taskgroup!");
      ompTimer ot{"TaskGroupSync"};

      OmpHappensAfter(Data->TaskGroup->GetPtr());

      // Delete this allocated taskgroup, all descendent task are finished by
      // now.
      Taskgroup *Parent = Data->TaskGroup->Parent;
      Data->TaskGroup->Delete();
      Data->TaskGroup = Parent;
      break;
    }

    case ompt_sync_region_reduction:
      // Should not occur according to OpenMP 5.1
      // Tested in OMPT tests
      break;
    }
    break;
  }
}

static void ompt_tsan_sync_region_wait(ompt_sync_region_t kind,
                                       ompt_scope_endpoint_t endpoint,
                                       ompt_data_t *parallel_data,
                                       ompt_data_t *task_data,
                                       const void *codeptr_ra) {
  TaskData *Data = ToTaskData(task_data);
  if (kind == ompt_sync_region_taskgroup) {
    switch (endpoint) {
    case ompt_scope_begin:
    case ompt_scope_beginend:
      if (analysis_flags->running)
        omptThreadCount->syncRegionBegin++;

      Data->InBarrier = true;
      thread_local_clock->enterState(STATE_OMP, "SyncRegionBegin");
      if (endpoint == ompt_scope_begin)
        break;
      KMP_FALLTHROUGH();
    case ompt_scope_end:
      if (analysis_flags->running)
        omptThreadCount->syncRegionEnd++;

      Data->InBarrier = false;
      thread_local_clock->exitState("SyncRegionEnd", Data->isRunning);
      break;
    }
  }
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
  TaskData *Data;
  DCHECK(new_task_data->ptr == NULL &&
         "Task data should be initialized to NULL");
  if (type & ompt_task_initial) {
    ompt_data_t *parallel_data;
    int team_size = 1;
    critical_ompt_get_parallel_info(0, &parallel_data, &team_size);
    ParallelData *PData = ParallelData::New(nullptr);
    parallel_data->ptr = PData;

    Data = TaskData::New(PData, type);
    new_task_data->ptr = Data;
  } else if (type & ompt_task_undeferred && !(type & ompt_task_target)) {
    Data = TaskData::New(ToTaskData(parent_task_data), type);
    new_task_data->ptr = Data;
  } else if (type & ompt_task_explicit || type & ompt_task_target) {
    Data = TaskData::New(ToTaskData(parent_task_data), type);
    new_task_data->ptr = Data;

    // Use the newly created address. We cannot use a single address from the
    // parent because that would declare wrong relationships with other
    // sibling tasks that may be created before this task is started!
    ompTimer ot{"TaskCreate"};
    OmpHappensBefore(Data->GetTaskPtr());
    ToTaskData(parent_task_data)->execution++;
  }
}

static void
ompt_tsan_task_creation(ompt_scope_endpoint_t endpoint,
                        ompt_data_t *parent_task_data, /* id of parent task */
                        ompt_data_t *new_task_data) /* id of created task    */
{
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

static void completeTask(TaskData *FromTask) {
  if (!FromTask)
    return;
  // Task-end happens after a possible omp_fulfill_event call
  if (FromTask->isFulfilled())
    OmpHappensAfter(FromTask->GetTaskPtr());
  // Included tasks are executed sequentially, no need to track
  // synchronization
  if (!FromTask->isIncluded()) {
    // Task will finish before a barrier in the surrounding parallel region
    // ...
    ParallelData *PData = FromTask->Team;
    OmpHappensBefore(PData->GetBarrierPtr(FromTask->BarrierIndex));

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

static void suspendTask(TaskData *FromTask, TaskData *ToTask) {
  if (!FromTask)
    return;
  // Task may be resumed at a later point in time.
  if (FromTask->isUntied()) {
    if (FromTask->InBarrier) {
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

static void switchTasks(TaskData *FromTask, TaskData *ToTask) {}

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
  } else if (ToTask->isUntied()) {
    if (ToTask->InBarrier) {
      thread_local_clock->exitState(STATE_OMP, STATE_OMP, "TaskContinueUntied",
                                    ToTask->isRunning);
    } else {
      OmpHappensAfter(ToTask->GetTaskPtr());
      thread_local_clock->enterState(STATE_USEFUL, "TaskContinueUntied");
    }
  } else {
    if (ToTask->InBarrier)
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

  if (analysis_flags->running && omptThreadCount)
    omptThreadCount->taskSchedule++;
  //
  //  The necessary action depends on prior_task_status:
  //
  //    ompt_task_early_fulfill = 5,
  //     -> ignored
  //
  //    ompt_task_late_fulfill  = 6,
  //     -> first completed, first freed, second ignored
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
    completeTask(FromTask);
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
    completeTask(FromTask);
    switchTasks(FromTask, ToTask);
    freeTask(FromTask);
    startTask(ToTask);
    return;
  case ompt_task_cancel:
    suspendTaskEnd(FromTask, ToTask);
    completeTask(FromTask);
    switchTasks(FromTask, ToTask);
    freeTask(FromTask);
    startTask(ToTask);
    return;
  case ompt_task_detach:
    suspendTaskEnd(FromTask, ToTask);
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
}

static void ompt_tsan_dependences(ompt_data_t *task_data,
                                  const ompt_dependence_t *deps, int ndeps) {
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
    for (int i = 0; i < ndeps; i++) {
      auto ret = Data->Parent->DependencyMap->insert(
          Pair<void *, DependencyData *>({deps[i].variable.ptr, nullptr}));
      if (ret.second) {
        ret.first->second = DependencyData::New();
      }
      new ((void *)(Data->Dependencies + i))
          TaskDependency(ret.first->second, deps[i].dependence_type);
    }

    // This callback is executed before this task is first started.
    OmpHappensBefore(Data->GetTaskPtr());
  }
}

static void ompt_tsan_mutex_acquire(ompt_mutex_t kind, unsigned int hint,
                                    unsigned int impl, ompt_wait_id_t wait_id,
                                    const void *codeptr_ra) {
  if (analysis_flags->running)
    omptThreadCount->mutexAcquire++;
  thread_local_clock->enterState(STATE_OMP, "MutexAcquire");
}

/// OMPT event callbacks for handling locking.
static void ompt_tsan_mutex_acquired(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                                     const void *codeptr_ra) {

  //    ompTimer ot{};
  // Acquire our own lock to make sure that
  // 1. the previous release has finished.
  // 2. the next acquire doesn't start before we have finished our release.
  LocksMutex.lock();
  auto InsertPair =
      Locks.insert(Pair<ompt_wait_id_t, Pair<std::mutex, ompt_tsan_clockid> *>(
          {wait_id, nullptr}));
  if (InsertPair.second) { // true on successfull insertion
    InsertPair.first->second = new Pair<std::mutex, ompt_tsan_clockid>();
  }
  // InsertPair iterator can get invalidated on rehash
  auto LockIdPair = InsertPair.first->second;
  LocksMutex.unlock();
  LockIdPair->first.lock();
  OmpHappensAfter(&LockIdPair->second);
  thread_local_clock->exitState("MutexAcquired");
}

static void ompt_tsan_mutex_released(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                                     const void *codeptr_ra) {
  ompTimer ot{"MutexRelease"};
  LocksMutex.lock();
  auto &Lock = Locks[wait_id];
  LocksMutex.unlock();
  OmpHappensBefore(&Lock->second);

  Lock->first.unlock();
}

static void ompt_tsan_device_initialize(int device_num, const char *type,
                                        ompt_device_t *device,
                                        ompt_function_lookup_t lookup,
                                        const char *documentation) {}

static void ompt_tsan_target_submit_emi(ompt_scope_endpoint_t endpoint,
                                        ompt_data_t *target_data,
                                        ompt_id_t *host_op_id,
                                        unsigned int requested_num_teams) {
  if (endpoint == ompt_scope_begin) {
    thread_local_taskdata = ToTaskData(target_data);
    if (!ToTaskData(target_data)->isMerged())
      target_data->ptr = nullptr;
#ifdef USE_CUDA
    PushCuptiCid((uint64_t)thread_local_taskdata);
#endif
  } else {
    thread_local_taskdata = nullptr;
#ifdef USE_CUDA
    PopCuptiCid();
#endif
  }
}

static void ompt_tsan_target_emi(ompt_target_t kind,
                                 ompt_scope_endpoint_t endpoint, int device_num,
                                 ompt_data_t *task_data,
                                 ompt_data_t *target_task_data,
                                 ompt_data_t *target_data,
                                 const void *codeptr_ra) {
  if (analysis_flags->verbose)
    printf("%s: %s, %s, %p, %p\n", __func__, ompt_target_t_values[kind],
           ompt_scope_endpoint_t_values[endpoint], target_task_data->ptr,
           target_data->ptr);

  TaskData *Data;

  if (endpoint == ompt_scope_begin) {
    // thread_local_clock->StopNested(CLOCK_OMP, "ompt_tsan_target_emi");
    thread_local_clock->enterState(STATE_OMP, __func__);

    if (kind != ompt_target && kind != ompt_target_nowait)
      return;

    if (!target_task_data || !target_task_data->ptr) { // without nowait
      Data = TaskData::New(ToTaskData(task_data),
                           ompt_task_target | ompt_task_merged,
                           thread_local_clock, device_num);
      GpuHappensBefore(Data->GetTaskPtr());
      target_data->ptr = Data;
    } else { // nowait
      Data = ToTaskData(target_task_data);
      Data->device_num = device_num;
      target_data->ptr = Data;
      target_task_data->ptr = nullptr;
    }
  }

  if (endpoint == ompt_scope_end) {
    // Data = ToTaskData(target_data);
    // if (Data && Data->isMerged()) {
    //   GpuHappensAfter(Data->GetTaskPtr());
    //   Data->freeTask();
    // }

    // thread_local_clock->StartNested(CLOCK_OMP, "ompt_tsan_target_emi");
    thread_local_clock->exitState(__func__);
  }
}

static void ompt_tsan_ompx_target_kernel(ompt_scope_endpoint_t endpoint,
                                         ompt_data_t *target_task_data,
                                         ompt_data_t *target_data) {
  if (analysis_flags->verbose)
    printf("%s: %s, %p, %p\n", __func__, ompt_scope_endpoint_t_values[endpoint],
           target_task_data->ptr, target_data->ptr);

  TaskData *Data;

  assert(target_data && target_data->ptr);
  Data = ToTaskData(target_data);

  if (endpoint == ompt_scope_begin) {
    Data->StartTime = getTime();
  } else if (endpoint == ompt_scope_end) {
    Data->finish(getTime() - Data->StartTime, -1);
  }
}

// callback , signature , variable to store result , required support level
#define SET_OPTIONAL_CALLBACK_T(event, type, result, level)                    \
  do {                                                                         \
    ompt_callback_##type##_t tsan_##event = &ompt_tsan_##event;                \
    result = ompt_set_callback((ompt_callbacks_t)ompt_callback_##event,        \
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

typedef void (*ompt_callback_ompx_target_kernel_t)(
    ompt_scope_endpoint_t endpoint, ompt_data_t *target_task_data,
    ompt_data_t *target_data);
static int ompt_callback_ompx_target_kernel = 38;

static int ompt_tsan_initialize(ompt_function_lookup_t lookup, int device_num,
                                ompt_data_t *tool_data) {
  if (!CheckRequestedAnalysis(AT_OMP)) {
    if (analysis_flags->verbose) {
      fprintf(analysis_flags->output,
              "OpenMP runtime detected, but no OpenMP analysis requested.\n");
    }
  }

  ompt_set_callback_t ompt_set_callback =
      (ompt_set_callback_t)lookup("ompt_set_callback");
  if (ompt_set_callback == NULL) {
    fprintf(stderr, "Could not set callback, exiting...\n");
    std::exit(1);
  }
  critical_ompt_get_parallel_info =
      (ompt_get_parallel_info_t)lookup("ompt_get_parallel_info");
  critical_ompt_get_thread_data =
      (ompt_get_thread_data_t)lookup("ompt_get_thread_data");
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
  SET_CALLBACK(task_schedule);
  SET_CALLBACK(dependences);

  SET_CALLBACK(mutex_acquire);
  SET_CALLBACK_T(mutex_acquired, mutex);
  SET_CALLBACK_T(mutex_released, mutex);

  // Target callbacks
  SET_CALLBACK(device_initialize);
  SET_CALLBACK(target_emi);
  int hasTargetKernel;
  SET_OPTIONAL_CALLBACK_T(ompx_target_kernel, ompx_target_kernel,
                          hasTargetKernel, ompt_set_never);
  if (!hasTargetKernel) {
    SET_CALLBACK(target_submit_emi);
  }

  ActivateAnalysis(AT_OMP);

  if (!thread_local_clock)
    thread_local_clock = new THREAD_CLOCK(my_next_id(), 0);
  startTool(false);
  return 1; // success
}

static void ompt_tsan_finalize(ompt_data_t *tool_data) {
  if (!CheckAnalysis(AT_MPI)) {
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
  if (!omp_thread_clocks)
    omp_thread_clocks = new Vector<THREAD_CLOCK *>{};
  if (!omp_thread_counts)
    omp_thread_counts = new Vector<omptCounts *>{};

  // The OMPT start-up code uses dlopen with RTLD_LAZY. Therefore, we cannot
  // rely on dlopen to fail if TSan is missing, but would get a runtime error
  // for the first TSan call. We use RunningOnValgrind to detect whether
  // an implementation of the Annotation interface is available in the
  // execution or disable the tool (by returning NULL).

  if (analysis_flags->verbose && analysis_flags->stopped)
    fprintf(analysis_flags->output,
            "Tool enabled, collecting critical path.\n");
  if (analysis_flags->verbose && !analysis_flags->stopped)
    fprintf(analysis_flags->output, "Tool enabled, waiting to get started.\n");
  return &ompt_start_tool_result;
}
