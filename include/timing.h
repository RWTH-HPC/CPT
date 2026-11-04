#include <math.h>
#include <string>
#include <time.h>

#define BILLION 1000000000

class RaiiTimer {
  class RaiiIterTimer {
    RaiiTimer *timer{nullptr};

  public:
    RaiiIterTimer(RaiiTimer *timer) : timer(timer) { timer->startIter(); }
    ~RaiiIterTimer() { timer->stopIter(); }
  };

  struct timespec start {};
  struct timespec istart {};
  double total{0}, sumsqr{0}, imin{1e10}, imax{0};
  int samples{0};
  std::string description{};

public:
  RaiiTimer(const std::string &descr) : description(descr) {
    clock_gettime(CLOCK_MONOTONIC, &start);
  }
  RaiiTimer(const char *descr) : description(descr) {
    clock_gettime(CLOCK_MONOTONIC, &start);
  }
  RaiiTimer(int id, const char *descr)
      : description(std::to_string(id) + ": " + std::string(descr)) {
    clock_gettime(CLOCK_MONOTONIC, &start);
  }
  RaiiIterTimer iterTimer() { return RaiiIterTimer{this}; }
  void startIter() { clock_gettime(CLOCK_MONOTONIC, &istart); }
  void stopIter() {
    struct timespec iend;
    clock_gettime(CLOCK_MONOTONIC, &iend);
    double startd =
        (double)(istart.tv_sec) + ((double)(istart.tv_nsec) / BILLION);
    double endd = (double)(iend.tv_sec) + ((double)(iend.tv_nsec) / BILLION);
    double elapsed = endd - startd;
    total += elapsed;
    sumsqr += elapsed * elapsed;
    if (elapsed < imin)
      imin = elapsed;
    if (elapsed > imax)
      imax = elapsed;
    samples++;
  }
  ~RaiiTimer() {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    double startd =
        (double)(start.tv_sec) + ((double)(start.tv_nsec) / BILLION);
    double endd = (double)(end.tv_sec) + ((double)(end.tv_nsec) / BILLION);
    double elapsed = endd - startd;
    if (samples == 0) {
      printf("%-25s elapsed = %1.9f\n", description.c_str(), elapsed);
      return;
    }
    double mean = total / samples;
    double stddev2 = (sumsqr / samples) - (mean * mean);
    double error = sqrt(stddev2);
    printf("%-25s total = %f, iter runtime (ms) = %f +/- %1.9f (%2.1f%%), min "
           "/ max (ms) = %f / %f\n",
           description.c_str(), total, mean * 1000, error * 1000,
           error / mean * 100, imin * 1000, imax * 1000);
  }
};
