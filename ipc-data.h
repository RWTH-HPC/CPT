#ifndef IPC_DATA_H
#define IPC_DATA_H 1

#include "cptConfig.h"
#include <cstdint>

#define NUM_UC_VALUES 1

struct depMetric {
  double fvalues[NUM_UC_DOUBLE +
                 1]; // first value is ref value, then double dependent values
  int64_t ivalues[NUM_UC_INT64]; // integer dependent values
};

#if NUM_UC_INT64 > 0 || NUM_UC_DOUBLE > 0
using ipcMetric = depMetric;
#else
using ipcMetric = double;
#endif

#endif