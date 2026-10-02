#ifndef FIBERPOOL_H
#define FIBERPOOL_H

#include <atomic>
#include <list>
#include <mutex>
#include <stdlib.h>
#include <unistd.h>

#include "containers.h"
#include "debug.h"
#include "parse_flags.h"
namespace __fiber {

static int pagesize{4096};
static std::atomic<int> fibers{0};

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
    // number of padded elements to allocate
    int ndatas = pagesize / paddedSize;
    char *datas = (char *)malloc(ndatas * paddedSize);
    memory.PushBack(datas);
    for (int i = 0; i < ndatas; i++) {
      DataPointer.PushBack(new (datas + i * paddedSize) T(this));
    }
    total += ndatas;
    // printf("%s, %li, %li, %i\n", __PRETTY_FUNCTION__, elemSize, paddedSize,
    //        ndatas);
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

template <typename T> class DataPoolEntry /*: public FiberApi*/ {
public:
  DataPool<T> *owner;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif
  static T *New() { return DataPool<T>::ThreadDataPool->getData(); }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  void Delete() {
    static_cast<T *>(this)->Reset();
    if (owner == DataPool<T>::ThreadDataPool)
      owner->returnOwnData(static_cast<T *>(this));
    else
      owner->returnData(static_cast<T *>(this));
  }

  DataPoolEntry(DataPool<T> *dp) : owner(dp) {}
};

#ifndef EMPTYCB
#define OmpGetCurrentFiber() __pool_get_current_fiber()
#define OmpCreateFiber(flag) __pool_create_fiber_dbg(flag, __FILE__, __LINE__)
#define OmpDestroyFiber(fiber) __pool_destroy_fiber(fiber)
#define OmpSwitchToFiber(fiber, flag)                                          \
  __pool_switch_to_fiber_dbg(fiber, flag, __FILE__, __LINE__)
#else
#define OmpGetCurrentFiber()
#define OmpCreateFiber(flag)
#define OmpDestroyFiber(fiber)
#define OmpSwitchToFiber(fiber, flag)
#endif
#define OmpFiberPoolInit __pool_init
#define OmpFiberPoolFini __pool_fini

} // namespace __fiber

extern "C" void *__pool_get_current_fiber();

extern "C" void *__pool_create_fiber_dbg(unsigned flags, const char *file,
                                         int line);

extern "C" void *__pool_create_fiber(unsigned flags);

extern "C" void __pool_destroy_fiber(void *fiber);

extern "C" void __pool_switch_to_fiber_dbg(void *fiber, unsigned flags,
                                           const char *file, int line);

extern "C" void __pool_switch_to_fiber(void *fiber, unsigned flags);

extern "C" void __pool_init();

extern "C" void __pool_fini();

#endif