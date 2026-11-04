// clang-format off
// RUN: %t 2>&1 | %FileCheck %s
// CHECK: Passed time verification
// clang-format on

#include "containers.h"
#include "timing.h"
#include <cstdio>
#include <random>
#include <set>

#define TRANSFER 10
#define KERNEL 100
#define STREAMS 10
#define SIZE 5
#define IDLE (KERNEL * STREAMS)
#define ENTRIES 100000

#define BASETYPE int

#include "cuda-tracking.h"

#define TIMELINE 4096

// Compares against painting the events onto a timeline. With evict set, the
// oldest intervals get aggregated mid-way, while the events are still arriving
// in time order, so nothing can arrive for the aggregated part.
static void CheckAgainstBruteForce(std::default_random_engine &e1, bool evict) {
  std::uniform_int_distribution<int> begin_dist(0, TIMELINE - 65);
  std::uniform_int_distribution<int> len_dist(1, 64);
  std::uniform_int_distribution<int> type_dist(0, 1);

  for (int run = 0; run < 200; run++) {
    EventSet<BASETYPE> set;
    DeviceTimes<BASETYPE> times;
    Vector<EventRecord<BASETYPE>> events;
    for (int i = 0; i < 256; i++) {
      BASETYPE begin = begin_dist(e1);
      events.push_back(
          EventRecord<BASETYPE>(type_dist(e1) ? EVENT_KERNEL : EVENT_TRANSFER,
                                begin, begin + len_dist(e1)));
    }
    if (evict)
      std::sort(events.begin(), events.end(), EventRecord<BASETYPE>::Cmp);

    bool kernel[TIMELINE] = {false}, transfer[TIMELINE] = {false};
    BASETYPE kernelSum = 0, transferSum = 0;
    int inserted = 0;
    for (auto &e : events) {
      bool *timeline = e.type == EVENT_KERNEL ? kernel : transfer;
      (e.type == EVENT_KERNEL ? kernelSum : transferSum) += e.end - e.begin;
      for (BASETYPE t = e.begin; t < e.end; t++)
        timeline[t] = true;
      set.InsertEvent(e);
      if (evict && ++inserted % 32 == 0)
        set.AggregateOldest(times, 4);
    }
    set.AggregateTimes(times);

    BASETYPE kernelUnion = 0, transferOnly = 0;
    for (int t = 0; t < TIMELINE; t++) {
      kernelUnion += kernel[t];
      transferOnly += transfer[t] && !kernel[t];
    }
    CHECK_EQ(times.kernel, kernelUnion);
    CHECK_EQ(times.transfer, transferOnly);
    CHECK_EQ(times.kernelSum, kernelSum);
    CHECK_EQ(times.transferSum, transferSum);
  }
}

// An event overlapping an aggregated interval only adds what is new
static void CheckEvictionKeepsOverlap() {
  EventSet<BASETYPE> set;
  DeviceTimes<BASETYPE> times;
  set.InsertEvent(EVENT_KERNEL, 0, 10);
  set.AggregateOldest(times, 0);
  set.InsertEvent(EVENT_KERNEL, 5, 15);
  set.AggregateTimes(times);
  CHECK_EQ(times.kernel, 15);
  CHECK_EQ(times.kernelSum, 20);
}

// Transfer time gets shaved off by other transfers as well as by kernels
static void CheckTransferOverlap() {
  EventSet<BASETYPE> set;
  DeviceTimes<BASETYPE> times;
  set.InsertEvent(EVENT_TRANSFER, 0, 10);
  set.InsertEvent(EVENT_TRANSFER, 5, 15);
  set.InsertEvent(EVENT_TRANSFER, 20, 30);
  set.InsertEvent(EVENT_KERNEL, 20, 25);
  set.AggregateTimes(times);
  CHECK_EQ(times.transfer, 20);
  CHECK_EQ(times.transferSum, 30);
}

int main(int argc, char **argv) {
  EventSet<BASETYPE> set;
  Vector<Vector<EventRecord<BASETYPE>>> intervalLists;
  intervalLists.resize(STREAMS);
  int entries = ENTRIES;
  if (argc > 1)
    entries = std::atoi(argv[1]);

  std::default_random_engine e1(1337);
  std::uniform_int_distribution<int> transfer_dist(TRANSFER, TRANSFER * SIZE);
  std::uniform_int_distribution<int> kernel_dist(KERNEL, KERNEL * SIZE);
  std::uniform_int_distribution<int> idle_dist(IDLE, IDLE * SIZE);
  for (int s = 0; s < STREAMS; s++) {
    BASETYPE start = 0, end = 0;
    BASETYPE idle = idle_dist(e1);
    start = end = idle * s / STREAMS;
    for (int i = 0; i < entries; i++) {
      BASETYPE idle = idle_dist(e1), kernel = kernel_dist(e1),
               transfer1 = transfer_dist(e1), transfer2 = transfer_dist(e1);
      end += transfer1;
      intervalLists[s].push_back(EventRecord(EVENT_TRANSFER, start, end));
      start += transfer1;
      end += kernel;
      intervalLists[s].push_back(EventRecord(EVENT_KERNEL, start, end));
      start += kernel;
      end += transfer2;
      intervalLists[s].push_back(EventRecord(EVENT_TRANSFER, start, end));
      start += transfer2;
      end += idle;
      start += idle;
    }
    printf("last elem: %d\n", end);
  }
  RaiiTimer prioTreeInsert(std::string("PrioIntervalset::insert"));
  for (int s = 0; s < STREAMS; s++) {
    printf("set.size=%li\n", set.Size());
    RaiiTimer prioTreeInsert1(std::string("PrioIntervalset::insert iter"));
    auto t1 = prioTreeInsert1.iterTimer();
    auto t = prioTreeInsert.iterTimer();
    for (auto &e : intervalLists[s]) {
      set.InsertEvent(e);
    }
  }
  printf("tree.size=%li\n", set.Size());

  auto *esptr = set.GetSet();
  EventRecord<BASETYPE> *prev = nullptr;

  for (auto &e : *esptr) {
    if (prev == nullptr) {
      prev = &e;
      continue;
    }

    if (e.type == prev->type) {
      CHECK_LT(prev->begin, e.begin);
      // touching intervals are not merged with their successor
      CHECK_LE(prev->end, e.begin);
    } else {
      CHECK_LE(prev->begin, e.begin);
      CHECK_LE(prev->end, e.begin);
    }

    prev = &e;
  }

  printf("Passed verification\n");

  CheckAgainstBruteForce(e1, false);
  CheckAgainstBruteForce(e1, true);
  CheckEvictionKeepsOverlap();
  CheckTransferOverlap();

  printf("Passed time verification\n");
}