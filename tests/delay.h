#include <math.h>
#include <sys/types.h>

#define VERBOSE 1

void delay_sleep(double delaylength);
double getclock();

void print_output(const char *app_name, int ntasks, double avg_task_time,
                  double taNCP, double thrCP, double runtime, double lbser,
                  double te, int sep_lb_te);