#include "delay.h"
#include <assert.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void delay_sleep(double delaylength) { usleep(delaylength); }

double getclock() {
  double time;
  // Returns a value in seconds of the time elapsed from some arbitrary,
  // but consistent point.
  double omp_get_wtime(void);
  time = omp_get_wtime();
  return time;
}

void print_output(const char *app_name, int ntasks, double avg_task_time,
                  double taNCP, double thrCP, double runtime, double lbser,
                  double te, int sep_lb_te) {

  FILE *of = stdout;

  fprintf(of, "App: %s\n", app_name);
  fprintf(of, "#tasks: %i\n", ntasks);
  fprintf(of, "AVG task time: %6.9lf s\n", avg_task_time);
  fprintf(of, "TaNCP: %6.9lf s\n", taNCP);
  fprintf(of, "G-CUE: %6.9lf s\n", thrCP);
  fprintf(of, "Runtime: %6.9lf s\n", runtime);
  if (sep_lb_te) {
    fprintf(of, "LB: %6.3lf\n", lbser);
    fprintf(of, "SerE: %6.3lf\n", 1.0);
  } else {
    fprintf(of, "LB: -\n");
    fprintf(of, "SerE: -\n");
  }
  fprintf(of, "Idle: %6.3lf\n", lbser);
  fprintf(of, "TE: %6.9lf\n", te);

  fclose(of);
}