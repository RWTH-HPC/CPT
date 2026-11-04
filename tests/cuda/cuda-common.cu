#include "cuda-common.h"
#include "debug.h"
#include <ctime>
#include <cuda.h>
#include <iostream>
#include <nvtx3/nvToolsExt.h>

constexpr size_t DEFAULT_MT_SIZE = 512;

CompactHashMap<int, DeviceTestContext> dtcs;

__global__ void gpuSleep(uint64_t duration_ns, uint64_t *ts = nullptr) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
  uint64_t start = __nano();
  while (__nano() - start < duration_ns)
    __nanosleep(1000);
#else
  uint64_t start = __nano();
  while (__nano() - start < duration_ns) {
  }
#endif
  if (ts) {
    ts[0] = start;
    ts[1] = __nano();
  }
}

__global__ void gpuTimestamp(uint64_t *ts) { *ts = __nano(); }

int GetNumDevices() {
  int deviceCountAvailable = 0;
  gpuErrchk(cudaGetDeviceCount(&deviceCountAvailable));
  return deviceCountAvailable;
}

void InitDevices(int deviceCountRequested, int rank, bool timeRef) {
  int deviceCountAvailable = 0;
  gpuErrchk(cudaGetDeviceCount(&deviceCountAvailable));

  if (deviceCountAvailable == 0) {
    return;
  }

  CHECK_VA(deviceCountAvailable >= deviceCountRequested,
           "More devices than available were requested.");
  int deviceCount = std::min(deviceCountAvailable, deviceCountRequested);

  for (int i = 0; i < deviceCount; i++) {
    int deviceIdx = (rank + i) % deviceCountAvailable;
    gpuErrchk(cudaSetDevice(deviceIdx));
    auto &currDevice = dtcs[i];
    currDevice.deviceId = deviceIdx;

    // initialize "memtransfer" part

    size_t freeBytes, totalBytes;
    gpuErrchk(cudaMemGetInfo(&freeBytes, &totalBytes));

    if (DEFAULT_MT_SIZE * 1e6 < freeBytes) {
      currDevice.mtc.size = DEFAULT_MT_SIZE * 1e6;
    } else {
      currDevice.mtc.size = (size_t)std::floor(0.75 * (double)freeBytes);
    }

    currDevice.mtc.h = (char *)malloc(currDevice.mtc.size);
    CHECK(currDevice.mtc.h);

    for (size_t j = 0; j < currDevice.mtc.size; j++) {
      currDevice.mtc.h[j] = 1;
    }

    gpuErrchk(cudaMalloc(&currDevice.mtc.d, currDevice.mtc.size));
  }
}

void CleanUpDevices() {
  for (auto &dp : dtcs) {
    auto currDeviceId = dp.first;
    auto &currDevice = dp.second;
    gpuErrchk(cudaSetDevice(currDeviceId));

    if (currDevice.mtc.h) {
      free(currDevice.mtc.h);
    }

    if (currDevice.mtc.d) {
      gpuErrchk(cudaFree(currDevice.mtc.d));
    }
  }
}

cudaStream_t CreateStream(int deviceId) {
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  gpuErrchk(cudaSetDevice(dtcs[deviceId].deviceId));
  cudaStream_t s;
  gpuErrchk(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
  return s;
}

void EnsureNStreams(int deviceId, int streams) {
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  int needed = streams - dtcs[deviceId].streams.size();
  for (int i = needed; i > 0; i--) {
    dtcs[deviceId].streams.push_back(CreateStream(deviceId));
  }
}

void SyncWorkStreams(int deviceId) {
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  for (auto s : dtcs[deviceId].streams) {
    gpuErrchk(cudaStreamSynchronize(s));
  }
  dtcs[deviceId].streamsSynced = true;
}

void DestroyStream(int deviceId, cudaStream_t s) {
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  gpuErrchk(cudaSetDevice(dtcs[deviceId].deviceId));
  gpuErrchk(cudaStreamDestroy(s));
}

void DoMemTransfer(uint64_t duration_us, int kind, int deviceId) {
  uint64_t duration_ns = duration_us * 1000;
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  auto &currDevice = dtcs[deviceId];

  gpuErrchk(cudaSetDevice(currDevice.deviceId));

  struct timespec start, current;
  clock_gettime(CLOCK_MONOTONIC, &start);
  current = start;

  uint64_t *time;

  if (dtcs[deviceId].doRef) {
    cudaHostAlloc(&time, 2 * sizeof(uint64_t), 0);
    gpuTimestamp<<<1, 1>>>(&time[0]);
  }

  while (TimespecToNs(current) < TimespecToNs(start) + duration_ns) {
    // for now only these two are supported
    if (kind == 0) {
      gpuErrchk(cudaMemcpyAsync(currDevice.mtc.h, currDevice.mtc.d,
                                currDevice.mtc.size, cudaMemcpyDeviceToHost));
    } else {
      gpuErrchk(cudaMemcpyAsync(currDevice.mtc.d, currDevice.mtc.h,
                                currDevice.mtc.size, cudaMemcpyHostToDevice));
    }
    clock_gettime(CLOCK_MONOTONIC, &current);
  }

  if (dtcs[deviceId].doRef) {
    gpuTimestamp<<<1, 1>>>(&time[1]);
  }
}

void DoWorkAsync(uint64_t duration_us, int deviceId, cudaStream_t stream) {
  uint64_t duration_ns = duration_us * 1000;
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  auto &currDevice = dtcs[deviceId];
  gpuErrchk(cudaSetDevice(currDevice.deviceId));
  gpuSleep<<<1, 1, 0, stream>>>(duration_ns);
}

void DoWorkNStreamAsync(uint64_t duration_us, int deviceId, int streams) {
  CHECK_NE(dtcs.find(deviceId), dtcs.end());
  for (auto s : dtcs[deviceId].streams) {
    DoWorkAsync(duration_us, deviceId, s);
  }
  dtcs[deviceId].streamsSynced = false;
}

void DoWork(uint64_t duration_us, int deviceId) {
  DoWorkAsync(duration_us, deviceId);
  // device already set by async func
  gpuErrchk(cudaDeviceSynchronize());
}

void DeviceSynchronize(int deviceId) {
  auto &currDevice = dtcs[deviceId];
  gpuErrchk(cudaSetDevice(currDevice.deviceId));
  gpuErrchk(cudaDeviceSynchronize());
}

static nvtxRangeId_t testRange = 0;

void BeginTest() { testRange = nvtxRangeStartA("CPTTEST"); }

void EndTest() { nvtxRangeEnd(testRange); }