#include "fiberpool.h"
#include "criticalPath.h"

#include <dlfcn.h>

namespace __fiber {

#ifdef DEBUG_FIBER
#define DEBUG_FIBER_INFO_ARGS(f, l) , f, l
#else
#define DEBUG_FIBER_INFO_ARGS(f, l)
#endif

// template <class T> struct threadClock;
using FiberDataPool = DataPool<THREAD_CLOCK>;

template <> __thread FiberDataPool *FiberDataPool::ThreadDataPool = nullptr;

} // namespace __fiber

extern "C" void *__pool_get_current_fiber() {
  return thread_local_clock->fiber;
}

// debug version
extern "C" void *__pool_create_fiber_dbg(unsigned flags, const char *file,
                                         int line) {
  return THREAD_CLOCK::New(0.0);
}

extern "C" void *__pool_create_fiber(unsigned flags) {
  return __pool_create_fiber_dbg(flags, NULL, 0);
}

extern "C" void __pool_destroy_fiber(void *fiber) {
  using namespace __fiber;
  reinterpret_cast<THREAD_CLOCK *>(fiber)->Delete();
}

// debug version
extern "C" void __pool_switch_to_fiber_dbg(void *fiber, unsigned flags,
                                           const char *file, int line) {
  using namespace __fiber;
  DCHECK(fiber);
  DCHECK(thread_local_clock);

  // update pointer to fiber clock
  thread_local_clock->fiber = reinterpret_cast<THREAD_CLOCK *>(fiber);
}

extern "C" void __pool_switch_to_fiber(void *fiber, unsigned flags) {
  __pool_switch_to_fiber_dbg(fiber, flags, NULL, 0);
}

namespace __fiber {

std::mutex InitMutex{};
extern "C" void __pool_init() {
  const std::lock_guard<std::mutex> lock(InitMutex);
  if (FiberDataPool::ThreadDataPool)
    return;
  pagesize = getpagesize();
  FiberDataPool::ThreadDataPool = new FiberDataPool;
}

extern "C" void __pool_fini() {
  if (FiberDataPool::ThreadDataPool)
    delete FiberDataPool::ThreadDataPool;
}
} // namespace __fiber