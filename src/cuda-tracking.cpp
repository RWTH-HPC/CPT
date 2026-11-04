#include "cuda-tracking.h"
#include "criticalPath.h"
#include "debug.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cupti.h>
#include <cupti_activity.h>
#include <cupti_callbacks.h>
#include <cupti_result.h>
#include <dlfcn.h>
#include <generated_nvtx_meta.h>
#include <nvtx3/nvToolsExt.h>
#include <stdio.h>

// #include "cupti-register.h"

#define CUPTI_API_CALL(apiFunctionCall)                                        \
  do {                                                                         \
    CUptiResult _status = apiFunctionCall;                                     \
    if (_status != CUPTI_SUCCESS) {                                            \
      const char *pErrorString;                                                \
      cuptiGetResultString(_status, &pErrorString);                            \
                                                                               \
      fprintf(stderr,                                                          \
              "%s:%d: Error: Function %s failed with error (%d): %s.\n",       \
              __FILE__, __LINE__, #apiFunctionCall, _status, pErrorString);    \
                                                                               \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

constexpr int MIN_EVENT_LIMIT = 1, MAX_EVENT_LIMIT = 30;
constexpr int MIN_CUPTI_BUFFER_LIMIT = 12, MAX_CUPTI_BUFFER_LIMIT = 30;

// cupti does not allow flushing from within an NVTX callback
// so range control needs to request finishMeasurement
static std::atomic<bool> finishPending{false};
CUpti_SubscriberHandle subscriberHandle;

static void RunPendingFinish() {
  if (finishPending.exchange(false))
    stopTool();
}

void cudaDeviceData::InitDevices() {
  if (!cudaDeviceDatas) {
    maxDevices = analysis_flags->device_limit;
    auto *arr = (std::atomic<cudaDeviceData *> *)malloc(
        maxDevices * sizeof(std::atomic<cudaDeviceData *>));
    for (int i = 0; i < maxDevices; i++) {
      new (&arr[i]) std::atomic<cudaDeviceData *>(nullptr);
    }
    cudaDeviceDatas = arr;
  }
}

// make sure to only call this after uninitcupti
void cudaDeviceData::FreeDevices() {
  if (!cudaDeviceDatas)
    return;

  for (int id = 0; id < maxDevices; id++) {
    cudaDeviceData *dev =
        cudaDeviceDatas[id].exchange(nullptr, std::memory_order_acq_rel);
    delete dev;
  }

  free(cudaDeviceDatas);
  cudaDeviceDatas = nullptr;
  maxDevices = 0;
}

cudaDeviceData *cudaDeviceData::GetOrCreateDevice(int deviceId) {
  CHECK_GE_VA(deviceId, 0, "Invalid device id.");
  CHECK_LT_VA(deviceId, maxDevices, "Maximum device number exceeded.");
  CHECK_VA(cudaDeviceDatas, "CUPTI tracking not properly initialized.");

  std::atomic<cudaDeviceData *> &slot = cudaDeviceDatas[deviceId];
  cudaDeviceData *dev = slot.load(std::memory_order_acquire);
  if (dev)
    return dev;

  auto *fresh = new cudaDeviceData();
  if (slot.compare_exchange_strong(dev, fresh, std::memory_order_acq_rel,
                                   std::memory_order_acquire))
    return fresh;

  delete fresh;

  return dev;
}

int cudaDeviceData::GetNumDevices() {
  if (!cudaDeviceDatas)
    return 0;
  int res = 0;
  for (int id = 0; id < maxDevices; id++) {
    cudaDeviceData *device =
        cudaDeviceDatas[id].load(std::memory_order_acquire);
    if (device)
      res++;
  }
  return res;
}

void cudaDeviceData::InsertEvent(EventType type, unsigned long begin,
                                 unsigned long end) {
  // CUPTI could not collect the timestamps, e.g. incomplete at a forced flush
  if (begin == CUPTI_TIMESTAMP_UNKNOWN || end <= begin)
    return;

  begin = std::max(begin, windowBegin.load(std::memory_order_relaxed));
  end = std::min(end, windowEnd.load(std::memory_order_relaxed));
  if (end <= begin)
    return;

  std::lock_guard<std::mutex> l(lock);
  // Records arrive out of order, so only ever aggregate the oldest intervals.
  // Aggregating everything would lose overlaps with records yet to arrive.
  if (eventSet.Size() >= maxEventRecords)
    eventSet.AggregateOldest(timesUnion, maxEventRecords / 2);
  eventSet.InsertEvent(type, begin, end);
  synchronized = false;
}

void cudaDeviceData::Synchronize() {
  std::lock_guard<std::mutex> l(lock);
  SynchronizeLocked();
}

void cudaDeviceData::SynchronizeLocked() {
  if (synchronized) {
    CHECK(eventSet.Size() == 0);
    return;
  }
  eventSet.AggregateTimes(timesUnion);
  eventSet.Reset();
  synchronized = true;
}

double cudaDeviceData::GetTimeKernel() { return NSTS(timesUnion.kernel); }

double cudaDeviceData::GetTimeTransfer() { return NSTS(timesUnion.transfer); }

double cudaDeviceData::GetTimeKernelOverlap() {
  return NSTS(timesUnion.kernelSum - timesUnion.kernel);
}

// all transfer time not counted, be it due to a kernel or another transfer
double cudaDeviceData::GetTimeTransferOverlap() {
  return NSTS(timesUnion.transferSum - timesUnion.transfer);
}

void cudaDeviceData::BeginWindow() {
  uint64_t timestamp = 0;
  CUPTI_API_CALL(cuptiGetTimestamp(&timestamp));
  windowEnd.store(std::numeric_limits<unsigned long>::max());
  windowBegin.store(timestamp);
}

void cudaDeviceData::EndWindow() {
  uint64_t timestamp = 0;
  CUPTI_API_CALL(cuptiGetTimestamp(&timestamp));
  windowEnd.store(timestamp);
}

void PushCuptiCid(uint64_t pushId) {
  cuptiActivityPushExternalCorrelationId(
      CUPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, pushId);
}
void PopCuptiCid() {
  uint64_t lastId = 0;
  CUPTI_API_CALL(cuptiActivityPopExternalCorrelationId(
      CUPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, &lastId));
}

static void HandleKernelLaunch(const CUpti_CallbackData *callbackInfo) {
  if (callbackInfo->callbackSite == CUPTI_API_ENTER) {
    GpuEnterRuntime();
    if (CheckAnalysis(AT_CUDA_CP)) {
      // Push the correlation ID to the activity API before kernel launch
      // TODO: get kernel data object address here
      // uint64_t pushId=(uint64_t) getKernelData();
      uint64_t pushId = (uint64_t)getKernelData();
      PushCuptiCid(pushId);

      if (analysis_flags->verbose) {
        fprintf(analysis_flags->output,
                "[CUPTI] Kernel launch: %s, correlationId %d, external "
                "correlationId: %lu\n",
                callbackInfo->symbolName, callbackInfo->correlationId, pushId);
      }
    }
  } else if (callbackInfo->callbackSite == CUPTI_API_EXIT) {
    // Remove correlation ID after kernel launch
    if (CheckAnalysis(AT_CUDA_CP)) {
      PopCuptiCid();
    }
    GpuExitRuntime();
  }
}

static void HandleSync(const CUpti_SynchronizeData *callbackInfo,
                       CUpti_CallbackId callbackId) {
  // Handle CUDA synchronization events
  GpuEnterRuntime();

  uint32_t streamId = 0;
  uint32_t deviceId = 0;

  if (CheckAnalysis(AT_CUDA_CP)) {
    // Flush all activity records to get kernel timings
    // TODO: not allowed from a synchronization callback according to CUPTI
    CUPTI_API_CALL(cuptiActivityFlushAll(0));

    // TODO: Terminate synchronization here
    if (callbackId == CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED) {
      CUPTI_API_CALL(cuptiGetStreamId(callbackInfo->context,
                                      callbackInfo->stream, &streamId));
      CUPTI_API_CALL(cuptiGetDeviceId(callbackInfo->context, &deviceId));
      GpuHappensAfter(getOrCreateStreamClock(streamId, deviceId));
    } else if (callbackId == CUPTI_CBID_SYNCHRONIZE_CONTEXT_SYNCHRONIZED) {
      CUPTI_API_CALL(cuptiGetDeviceId(callbackInfo->context, &deviceId));
      GpuSyncDevice(deviceId);
    }
    // terminateSync();
  }
  GpuExitRuntime();
}

static void HandleCudaRuntimeCall(const CUpti_CallbackData *cbInfo) {
  if (cbInfo->callbackSite == CUPTI_API_ENTER) {
    GpuEnterRuntime();

  } else if (cbInfo->callbackSite == CUPTI_API_EXIT) {
    GpuExitRuntime();
  }
}

static void CUPTIAPI CuptiCallbackHandler(void *userData,
                                          CUpti_CallbackDomain domain,
                                          CUpti_CallbackId callbackId,
                                          const void *callbackData) {

  switch (domain) {
  case CUPTI_CB_DOMAIN_RUNTIME_API: {
    // not a thread the tool knows about
    if (!thread_local_clock)
      break;
    const CUpti_CallbackData *callbackInfo =
        (const CUpti_CallbackData *)callbackData;
    switch (callbackId) {
    case CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernel_v7000:
      HandleKernelLaunch(callbackInfo);
      break;
    default:
      // TODO more events that should be caught
      HandleCudaRuntimeCall(callbackInfo);
      break;
    }
    if (callbackInfo->callbackSite == CUPTI_API_EXIT)
      RunPendingFinish();
    break;
  }
  case CUPTI_CB_DOMAIN_SYNCHRONIZE: {
    if (!thread_local_clock)
      break;
    const CUpti_SynchronizeData *callbackInfo =
        (const CUpti_SynchronizeData *)callbackData;
    switch (callbackId) {
    // Context synchronized (device)
    case CUPTI_CBID_SYNCHRONIZE_CONTEXT_SYNCHRONIZED:
      HandleSync(callbackInfo, callbackId);
      break;
    case CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED:
      HandleSync(callbackInfo, callbackId);
      // Stream synchronized
      break;
    default:
      break;
    }
    break;
  }
  case CUPTI_CB_DOMAIN_NVTX: {
    // only the first CPT range controls the tool
    static std::atomic<nvtxRangeId_t> cptRangeId{0};
    const CUpti_NvtxData *nvtxData = (const CUpti_NvtxData *)callbackData;
    if (callbackId == CUPTI_CBID_NVTX_nvtxRangeStartA) {
      const char *message =
          ((const nvtxRangeStartA_params *)nvtxData->functionParams)->message;
      // the user has to start the tool with a message starting with CPT
      if (!message || strncmp(message, "CPT", 3) != 0)
        break;
      nvtxRangeId_t expected = 0;
      if (!cptRangeId.compare_exchange_strong(
              expected, *(const nvtxRangeId_t *)nvtxData->functionReturnValue))
        break;
      if (!thread_local_clock) {
        thread_local_clock = new THREAD_CLOCK(my_next_id(), 0);
      }
      startTool();
    } else if (callbackId == CUPTI_CBID_NVTX_nvtxRangeEnd) {
      nvtxRangeId_t expected =
          ((const nvtxRangeEnd_params *)nvtxData->functionParams)->id;
      if (!expected || !cptRangeId.compare_exchange_strong(expected, 0))
        break;
      finishPending = analysis_flags->dump_on_stop && !CheckAnalysis(AT_MPI);
      stopTool(true);
      if (analysis_flags->verbose && analysis_flags->dump_on_stop) {
        fprintf(analysis_flags->output,
                "Could not dump on stop, cupti does not allow it with nvtx "
                "range control, will dump at next possible spot, library "
                "destructor at the latest\n");
      }
    }
    break;
  }
  default:
    break;
  }
}

static void CUPTIAPI BufferRequested(uint8_t **buffer, size_t *size,
                                     size_t *maxNumRecords) {
  // Allocate buffer for activity records
  *size = cuptiActivityBufferSize;
  *maxNumRecords = 0;
  *buffer = (uint8_t *)malloc(*size);
}

static void CUPTIAPI BufferCompleted(CUcontext ctx, uint32_t streamId,
                                     uint8_t *buffer, size_t size,
                                     size_t validSize) {
  // Process the activity records in the buffer
  CUpti_Activity *record = NULL;
  CUptiResult status = CUPTI_SUCCESS;
  uint64_t lastExternalCorrelationId = 0;
  uint64_t lastCorrelationId = 0;

  size_t dropped = 0;
  cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped);

  droppedCuptiRecords += dropped;

  while (1) {
    status = cuptiActivityGetNextRecord(buffer, validSize, &record);
    if (status == CUPTI_SUCCESS) {
      // Process the record
      if (record->kind == CUPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION) {
        if (!CheckAnalysis(AT_CUDA_CP)) {
          continue;
        }
        // external correlation record, store IDs since the next record will be
        // the matching kernel activity
        CUpti_ActivityExternalCorrelation *correlationRecord =
            (CUpti_ActivityExternalCorrelation *)record;
        lastExternalCorrelationId = correlationRecord->externalId;
        lastCorrelationId = correlationRecord->correlationId;

        if (analysis_flags->verbose) {
          fprintf(analysis_flags->output,
                  "[CUPTI] External: correlationId: %d, externalId %lu\n",
                  correlationRecord->correlationId,
                  correlationRecord->externalId);
        }

      } else if (record->kind == CUPTI_ACTIVITY_KIND_KERNEL ||
                 record->kind == CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL) {
        // Kernel activity record, get timing data
        CUpti_ActivityKernel9 *kernelRecord = (CUpti_ActivityKernel9 *)record;
        if (CheckAnalysis(AT_CUDA)) {
          cudaDeviceData::GetOrCreateDevice(kernelRecord->deviceId)
              ->InsertEvent(EVENT_KERNEL, kernelRecord->start,
                            kernelRecord->end);
        }

        if (!CheckAnalysis(AT_CUDA_CP)) {
          continue;
        }

        if (kernelRecord->correlationId != lastCorrelationId) {
          fprintf(analysis_flags->output,
                  "[CUPTI] Error: correlationId mismatch, this should never "
                  "happen: %d != %lu\n",
                  kernelRecord->correlationId, lastCorrelationId);
          continue;
        }

        if (analysis_flags->verbose) {
          fprintf(analysis_flags->output,
                  "[CUPTI] Kernel completed: %s, correlationId %d, stream %d,"
                  "start: %lu, end: %lu, diff: %lu\n",
                  kernelRecord->name, kernelRecord->correlationId,
                  kernelRecord->streamId, kernelRecord->start,
                  kernelRecord->end, kernelRecord->end - kernelRecord->start);
        }

        uint64_t duration = kernelRecord->end - kernelRecord->start;

        // TODO: Call kernel data here
        auto kernelData = (KernelData *)lastExternalCorrelationId;
        kernelData->finish(1.e-9 * duration, kernelRecord->streamId,
                           kernelRecord->deviceId);
      } else if (record->kind == CUPTI_ACTIVITY_KIND_MEMCPY ||
                 record->kind == CUPTI_ACTIVITY_KIND_MEMCPY2 ||
                 record->kind == CUPTI_ACTIVITY_KIND_MEMSET) {
        if (!CheckAnalysis(AT_CUDA)) {
          continue;
        }
        CUpti_ActivityMemcpy *memCpyRecord = (CUpti_ActivityMemcpy *)record;
        cudaDeviceData::GetOrCreateDevice(memCpyRecord->deviceId)
            ->InsertEvent(EVENT_TRANSFER, memCpyRecord->start,
                          memCpyRecord->end);
      }
    } else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED) {
      break;
    } else {
      printf("[CUPTI] Error processing activity records: %d\n", status);
      break;
    }
  }
  free(buffer);
}

void InitCUPTI() {
  // NVTX range support
  // if something else already uses NVTX ranges, they cannot be used for tool
  // control
  Dl_info info;
  const char *injection = getenv("NVTX_INJECTION64_PATH");
  if (dladdr((void *)&cuptiSubscribe, &info)) {
    if (!injection) {
      setenv("NVTX_INJECTION64_PATH", info.dli_fname, 0);
    } else if (!strstr(injection, "libcupti")) {
      fprintf(
          analysis_flags->output,
          "NVTX_INJECTION64_PATH=%s, NVTX ranges will not control the tool\n",
          injection);
    }
  }

  CUptiResult _status = cuptiSubscribe(
      &subscriberHandle, (CUpti_CallbackFunc)CuptiCallbackHandler, nullptr);
  if (_status == CUPTI_ERROR_NOT_INITIALIZED) {
    fprintf(stderr,
            "%s:%d: No compatible cuda library found for use with CUPTI\n",
            __FILE__, __LINE__);
    analysis_flags->cuda_analysis = false;
    return;
  }
  if (_status == CUPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED) {
    fprintf(stderr,
            "%s:%d: Another tool is using CUPTI. Ignoring CUPTI in OTF-CPT in "
            "this run.\n",
            __FILE__, __LINE__);
    analysis_flags->cuda_analysis = false;
    return;
  }
  if (_status != CUPTI_SUCCESS) {
    const char *pErrorString;
    cuptiGetResultString(_status, &pErrorString);

    fprintf(stderr, "%s:%d: Error: Function %s failed with error (%d): %s.\n",
            __FILE__, __LINE__,
            "cuptiSubscribe(&subscriberHandle, "
            "(CUpti_CallbackFunc)CuptiCallbackHandler, nullptr))",
            _status, pErrorString);

    exit(EXIT_FAILURE);
  }

  ActivateAnalysis(AT_CUDA | AT_CUDA_CP);

  int eventLimit = analysis_flags->event_limit;
  int cuptiBufferLimit = analysis_flags->cupti_buffer_limit;

  eventLimit = std::clamp(eventLimit, MIN_EVENT_LIMIT, MAX_EVENT_LIMIT);
  cuptiBufferLimit = std::clamp(cuptiBufferLimit, MIN_CUPTI_BUFFER_LIMIT,
                                MAX_CUPTI_BUFFER_LIMIT);
  maxEventRecords = size_t{1} << eventLimit;
  cuptiActivityBufferSize = size_t{1} << cuptiBufferLimit;

  if (CheckAnalysis(AT_CUDA))
    cudaDeviceData::InitDevices();

  // register all runtime API callbacks
  // register_cupti_runtime_callbacks(&subscriberHandle);
  CUPTI_API_CALL(
      cuptiEnableDomain(1, subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API));

  // Enable CUPTI callbacks on device / stream synchronization
  CUPTI_API_CALL(
      cuptiEnableCallback(1, subscriberHandle, CUPTI_CB_DOMAIN_SYNCHRONIZE,
                          CUPTI_CBID_SYNCHRONIZE_CONTEXT_SYNCHRONIZED));
  CUPTI_API_CALL(
      cuptiEnableCallback(1, subscriberHandle, CUPTI_CB_DOMAIN_SYNCHRONIZE,
                          CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED));

  CUPTI_API_CALL(cuptiEnableDomain(1, subscriberHandle, CUPTI_CB_DOMAIN_NVTX));

  // Acvitiy API, callbacks for buffer request and complete for buffer
  // requests and for buffers completed by CUPTI.
  CUPTI_API_CALL(
      cuptiActivityRegisterCallbacks(BufferRequested, BufferCompleted));
  // Tracking kernel times
  if (CheckRequestedAnalysis(AT_CUDA_CP)) {
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_KERNEL));
    if (analysis_flags->verbose) {
      fprintf(analysis_flags->output, "CUDA critical path analysis requested, "
                                      "kernel execution will be serialized.\n");
    }
    if (CheckAnalysis(AT_CUDA)) {
      fprintf(stderr, "CUDA critical path analysis serializes kernels, kernel "
                      "overlap will not be representative.\n");
    }
  } else {
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  }
  // Tracking memory transfer times
  if (CheckAnalysis(AT_CUDA)) {
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY2));
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMSET));
  }
  // Required for external correlation
  if (CheckRequestedAnalysis(AT_CUDA_CP)) {
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_RUNTIME));
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DRIVER));
    CUPTI_API_CALL(
        cuptiActivityEnable(CUPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION));
    CUPTI_API_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MARKER));
  }

  if (analysis_flags->verbose) {
    fprintf(analysis_flags->output, "CUPTI initialized.\n");
  }
}

void DeInitCUPTI() {
  if (!CheckAnalysis(AT_CUDA | AT_CUDA_CP)) {
    return;
  }
  CUPTI_API_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_FLUSH_FORCED));
  if (CheckAnalysis(AT_CUDA)) {
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY2));
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMSET));
  }
  if (CheckAnalysis(AT_CUDA_CP)) {
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_KERNEL));
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_RUNTIME));
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_DRIVER));
    CUPTI_API_CALL(
        cuptiActivityDisable(CUPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION));
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MARKER));
  } else {
    CUPTI_API_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  }
  CUPTI_API_CALL(cuptiUnsubscribe(subscriberHandle));

  // Synchronize all devices
  cudaDeviceData::ForEach(
      [&](int id, cudaDeviceData &device) { device.Synchronize(); });

  size_t droppedCupti = droppedCuptiRecords.load();
  if (droppedCupti > 0) {
    int recommendedParam = std::min(
        MAX_CUPTI_BUFFER_LIMIT, analysis_flags->cupti_buffer_limit +
                                    (int)std::ceil(std::log2(droppedCupti)));
    fprintf(analysis_flags->output,
            "Warning: CUPTI dropped %zu activity records, device times are "
            "incomplete.\nUse cupti_buffer_limit=%d in CPT_OPTIONS\n",
            droppedCupti, recommendedParam);
  }

  size_t late = lateEvents.load(), total = insertedEvents.load();
  if (late > 0) {
    int recommendedParam =
        std::min(MAX_EVENT_LIMIT,
                 analysis_flags->event_limit + (int)std::ceil(std::log2(late)));
    fprintf(analysis_flags->output,
            "Warning: EventSet clipped %zu late event records, device times "
            "are incomplete.\nUse event_limit=%d in CPT_OPTIONS\n",
            late, recommendedParam);
    if (analysis_flags->verbose) {
      fprintf(analysis_flags->output,
              "EventSet late records (maxEventRecords=%zu): %zu"
              " of %zu (%.4f%%), fully dropped %zu"
              ", clipped time %.6f s of %.6f s (%.4f%%)\n",
              maxEventRecords, late, total, total ? 100. * late / total : 0.,
              droppedEvents.load(), NSTS(clippedTime.load()),
              NSTS(insertedTime.load()),
              insertedTime.load()
                  ? 100. * clippedTime.load() / insertedTime.load()
                  : 0.);
    }
  }
}

// Enable CUPTI callback on kernel launch via driver API, this also gives us
// the stream
// CUPTI_API_CALL(cuptiEnableCallback(1, subscriberHandle,
//   CUPTI_CB_DOMAIN_DRIVER_API,
//   CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel));
// if (domain == CUPTI_CB_DOMAIN_DRIVER_API) {
//   switch (callbackId) {
//   case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel:
//   case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel_ptsz:
//   case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernelEx:
//   case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernelEx_ptsz: {
//     auto *params = (cuLaunchKernel_params *)callbackInfo->functionParams;
//     CUstream hStream = params->hStream;
//     if (callbackInfo->callbackSite == CUPTI_API_ENTER) {

//       printf(
//           "[CUPTI] Kernel launch started: %s, correlationId %d, stream
//           %p\n", callbackInfo->symbolName, callbackInfo->correlationId,
//           hStream);
//     } else if (callbackInfo->callbackSite == CUPTI_API_EXIT) {
//       printf(
//           "[CUPTI] Kernel launch stopped: %s, correlationId %d, stream
//           %p\n", callbackInfo->symbolName, callbackInfo->correlationId,
//           hStream);
//     }
//     break;
//   }
//   default:
//     break;
//   }