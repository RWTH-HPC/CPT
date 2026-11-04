#ifndef CUDA_COMMON_H
#define CUDA_COMMON_H

#include <cuda_runtime.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/containers.h"
#include "cuda-tracking.h"

#define TYPE double

#define gpuErrchk(ans) gpuAssert((ans), __FILE__, __LINE__)
inline void gpuAssert(cudaError_t code, const char *file, int line,
                      bool abort = true) {
  if (code != cudaSuccess) {
    fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file,
            line);
    if (abort)
      exit(code);
  }
}

struct MemTransferContext {
  size_t size{0};
  char *h{nullptr};
  char *d{nullptr};
};

struct DeviceTestContext {
  int deviceId{0};
  bool streamsSynced{true};
  bool doRef{false};
  EventSet<uint64_t> reference;
  Vector<cudaStream_t> streams;
  MemTransferContext mtc;
};

extern CompactHashMap<int, DeviceTestContext> dtcs;

#ifdef __CUDACC__
static __device__ __inline__ uint64_t __nano() {
  uint64_t mclk;
  asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(mclk));
  return mclk;
}

static __device__ __inline__ uint32_t __myclock() {
  uint32_t mclk;
  asm volatile("mov.u32 %0, %%clock;" : "=r"(mclk));
  return mclk;
}

#endif // __CUDACC__

static inline uint64_t TimespecToNs(const struct timespec &ts) {
  return (static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL) + ts.tv_nsec;
}

// Functions to be used by tests
int GetNumDevices();
void InitDevices(int deviceCountRequested = 1, int rank = 0,
                 bool timeRef = false);
void CleanUpDevices();
void DoWorkAsync(uint64_t duration_us, int deviceId = 0,
                 cudaStream_t stream = nullptr);
void DoWork(uint64_t duration_us, int deviceId = 0);
void DoMemTransfer(uint64_t duration_us, int kind, int deviceId = 0);
void EnsureNStreams(int deviceId, int streams);
void DoWorkNStreamAsync(uint64_t duration_us, int deviceId, int streams);
void SyncWorkStreams(int deviceId);

// Not per-device
void DeviceSynchronize(int deviceId = 0);

// Tool Control
void BeginTest();
void EndTest();

#endif // CUDA_COMMON_H