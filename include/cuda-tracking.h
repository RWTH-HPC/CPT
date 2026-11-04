#ifndef CUDA_TRACKING_H
#define CUDA_TRACKING_H

#include "containers.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <inttypes.h>
#include <limits>
#include <mutex>

#define NSTS(t) (1.e-9 * (t))

inline size_t maxEventRecords{1 << 12};
inline size_t cuptiActivityBufferSize{1 << 20};

// EventSet diag: records arriving below the aggregation floor
inline std::atomic<size_t> insertedEvents{0};
inline std::atomic<size_t> lateEvents{0};    // begin < floor, clipped
inline std::atomic<size_t> droppedEvents{0}; // entirely < floor
inline std::atomic<size_t> insertedTime{0};
inline std::atomic<size_t> clippedTime{0}; // time cut off by the floor

// CUPTI diag
inline std::atomic<size_t> droppedCuptiRecords{0};

void InitCUPTI();
void DeInitCUPTI();
void PushCuptiCid(uint64_t);
void PopCuptiCid();

enum EventType { EVENT_ERROR = -1, EVENT_KERNEL = 0, EVENT_TRANSFER = 1 };

template <typename T> struct DeviceTimes {
  T kernel{0};
  T transfer{0};
  T kernelSum{0};
  T transferSum{0};
};

template <typename T> struct EventRecord {
  EventType type{EVENT_KERNEL};
  T begin{0};
  T end{0};
  EventRecord() {}
  EventRecord(EventType t, T b, T e) : type(t), begin(b), end(e) {}
  EventRecord<T> &operator=(const EventRecord<T> &other);
  bool operator==(const EventRecord<T> &other) {
    return (type == other.type && begin == other.begin && end == other.end);
  }
  static bool Cmp(const EventRecord<T> &e1, const EventRecord<T> &e2);
  static bool Overlap(const EventRecord<T> &e1, const EventRecord<T> &e2);
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }
};

template <typename T> class EventSet {
public:
  EventSet<T>() : es(EventRecord<T>::Cmp) {}
  bool InsertEvent(EventRecord<T> &&ernew);
  bool InsertEvent(EventRecord<T> &ernew);
  bool InsertEvent(EventType type, T begin, T end);
  void AggregateTimes(DeviceTimes<T> &times);
  // aggregate intervals up to keep to not overload eventset
  void AggregateOldest(DeviceTimes<T> &times, size_t keep);
  void Reset();
  size_t Size() { return es.size(); }
  Set<EventRecord<T>, decltype(EventRecord<T>::Cmp) *> *GetSet() { return &es; }
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }

private:
  Set<EventRecord<T>, decltype(EventRecord<T>::Cmp) *> es;
  // we track this sum to get the overlap time
  T kernelSum{0};
  T transferSum{0};
  // everything before this has already been aggregated
  T floor{0};
};

class cudaDeviceData {
public:
  void InsertEvent(EventType type, unsigned long begin, unsigned long end);
  void Synchronize();
  double GetTimeKernel();
  double GetTimeTransfer();
  double GetTimeKernelOverlap();
  double GetTimeTransferOverlap();
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }

  static void InitDevices();
  static void FreeDevices();
  static cudaDeviceData *GetOrCreateDevice(int deviceId);

  static int GetNumDevices();

  // toolStart/end timestamps in cupti time
  static void BeginWindow();
  static void EndWindow();

  // enables us to iterate over all devices with a lambda
  template <typename F> static void ForEach(F &&fn) {
    if (!cudaDeviceDatas)
      return;
    for (int id = 0; id < maxDevices; id++) {
      cudaDeviceData *device =
          cudaDeviceDatas[id].load(std::memory_order_acquire);
      if (device)
        fn(id, *device);
    }
  }

private:
  bool synchronized{true};
  void SynchronizeLocked();

  DeviceTimes<unsigned long> timesUnion{0};

  // lock event insertion and synchronization
  std::mutex lock;
  EventSet<unsigned long> eventSet;

  inline static size_t maxDevices{16};
  inline static std::atomic<cudaDeviceData *> *cudaDeviceDatas{nullptr};

  // nothing is recorded until the measurement started
  inline static std::atomic<unsigned long> windowBegin{
      std::numeric_limits<unsigned long>::max()};
  inline static std::atomic<unsigned long> windowEnd{
      std::numeric_limits<unsigned long>::max()};
};

template <typename T>
EventRecord<T> &EventRecord<T>::operator=(const EventRecord<T> &other) {
  type = other.type;
  begin = other.begin;
  end = other.end;
  return *this;
}

template <typename T>
bool EventRecord<T>::Cmp(const EventRecord<T> &a, const EventRecord<T> &b) {
  return a.begin < b.begin; // sort by start time
}

template <typename T>
bool EventRecord<T>::Overlap(const EventRecord<T> &a, const EventRecord<T> &b) {
  return a.begin < b.end && b.begin < a.end;
}

template <typename T>
bool EventSet<T>::InsertEvent(EventType type, T begin, T end) {
  return InsertEvent(EventRecord<T>(type, begin, end));
}

template <typename T> bool EventSet<T>::InsertEvent(EventRecord<T> &ernew) {
  return InsertEvent(EventRecord<T>(ernew));
}

template <typename T> bool EventSet<T>::InsertEvent(EventRecord<T> &&ernew) {
  auto it = es.lower_bound(ernew);

  // get predecessor because it can still overlap
  // but only the first predecessor can overlap
  // due to the set being disjointed
  if (it != es.begin()) {
    auto prev = it;
    --prev;
    if (prev->end >= ernew.begin) {
      it = prev;
    }
  }

  // Record time into sums for overlap tracking
  T duration = ernew.end - ernew.begin;
  if (ernew.type == EVENT_KERNEL) {
    kernelSum += duration;
  } else if (ernew.type == EVENT_TRANSFER) {
    transferSum += duration;
  }

  insertedEvents.fetch_add(1, std::memory_order_relaxed);
  insertedTime.fetch_add(duration, std::memory_order_relaxed);
  if (ernew.begin < floor) {
    lateEvents.fetch_add(1, std::memory_order_relaxed);
    if (ernew.end <= floor)
      droppedEvents.fetch_add(1, std::memory_order_relaxed);
    clippedTime.fetch_add(std::min(ernew.end, floor) - ernew.begin,
                          std::memory_order_relaxed);
  }

  // already aggregated time must not be counted again
  ernew.begin = std::max(ernew.begin, floor);
  if (ernew.end <= ernew.begin)
    return true;

  // overlapping events to be readded
  Vector<EventRecord<T>> readd;

  // ernew(old) is event record new (old)
  while (it != es.end() && it->begin < ernew.end) {
    EventRecord<T> erold = *it;

    // erase and advance iterator
    it = es.erase(it);

    if (ernew.type == EVENT_KERNEL) {
      if (erold.type == EVENT_KERNEL) {
        // merge kernels
        ernew.begin = std::min(ernew.begin, erold.begin);
        ernew.end = std::max(ernew.end, erold.end);
      } else if (erold.type == EVENT_TRANSFER) {
        // remove chunk of memtransfer
        // readd whatever is left which could be
        // 2, 1 or 0 chunks
        if (erold.begin < ernew.begin)
          readd.push_back(
              EventRecord<T>(EVENT_TRANSFER, erold.begin, ernew.begin));
        if (erold.end > ernew.end)
          readd.push_back(EventRecord<T>(EVENT_TRANSFER, ernew.end, erold.end));
      }
    } else if (ernew.type == EVENT_TRANSFER) {
      if (erold.type == EVENT_TRANSFER) {
        // merge trasnfers
        ernew.begin = std::min(ernew.begin, erold.begin);
        ernew.end = std::max(ernew.end, erold.end);
      } else if (erold.type == EVENT_KERNEL) {
        // left chunk of memtransfer gets added
        if (ernew.begin < erold.begin) {
          readd.push_back(
              EventRecord<T>(EVENT_TRANSFER, ernew.begin, erold.begin));
        }
        // if anything is left to the right it becomes the new ernew
        if (ernew.end > erold.end) {
          ernew.begin = erold.end;
        } else { // if not we break
          ernew.type = EVENT_ERROR;
          readd.push_back(erold);
          break;
        }
        readd.push_back(erold);
      }
    }
  }

  if (ernew.type != EVENT_ERROR) {
    es.insert(ernew);
  }
  for (size_t i = 0; i < readd.size(); i++) {
    es.insert(readd[i]);
  }

  return true;
}

template <typename T> void EventSet<T>::AggregateTimes(DeviceTimes<T> &times) {
  for (auto e : es) {
    if (e.type == EVENT_KERNEL) {
      times.kernel += (e.end - e.begin);
    } else if (e.type == EVENT_TRANSFER) {
      times.transfer += (e.end - e.begin);
    }
  }
  times.kernelSum += kernelSum;
  times.transferSum += transferSum;
}

template <typename T>
void EventSet<T>::AggregateOldest(DeviceTimes<T> &times, size_t keep) {
  auto it = es.begin();
  while (es.size() > keep) {
    if (it->type == EVENT_KERNEL) {
      times.kernel += (it->end - it->begin);
    } else if (it->type == EVENT_TRANSFER) {
      times.transfer += (it->end - it->begin);
    }
    floor = std::max(floor, it->end);
    it = es.erase(it);
  }
  times.kernelSum += kernelSum;
  times.transferSum += transferSum;
  kernelSum = 0;
  transferSum = 0;
}

template <typename T> void EventSet<T>::Reset() {
  es.clear();
  kernelSum = 0;
  transferSum = 0;
  floor = 0;
}

// extern CompactHashMap<uint32_t, cudaDeviceData> *deviceDataMap;

#endif // CUDA_TRACKING_H
