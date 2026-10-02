#ifndef DEPENDENTMETRICS_C
#define DEPENDENTMETRICS_C 1

#include "dependentMetrics.h"
#include "criticalPath.h"
#include "parse_flags.h"
#include <sstream>

// Static member initialization
DepMetricOrganizer *DepMetricOrganizer::instancePtr = nullptr;
std::mutex DepMetricOrganizer::mtx;

TaskTypeMetricHandler *TaskTypeMetricHandler::instancePtr = nullptr;
std::mutex TaskTypeMetricHandler::mtx;
thread_local int taskTypeForDep = ompt_task_initial;

TaskPrioMetricHandler *TaskPrioMetricHandler::instancePtr = nullptr;
std::mutex TaskPrioMetricHandler::mtx;
thread_local int taskPrioForDep = 0;

TaskNameMetricHandler *TaskNameMetricHandler::instancePtr = nullptr;
std::mutex TaskNameMetricHandler::mtx;
thread_local std::string taskNameForDep = "";

TaskCountMetricHandler *TaskCountMetricHandler::instancePtr = nullptr;
std::mutex TaskCountMetricHandler::mtx;
thread_local uint64_t taskCountForDep = 0;
/*
 * Task type metrics
 */

TaskTypeMetricHandler::TaskTypeMetricHandler() {
  description = "Task Type Share on CP";
  numValues = 5;

  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Task type metrics available.\n");
}

void TaskTypeMetricHandler::getValues(Array<double, NUM_UC_DOUBLE> *res,
                                      int spos, int epos) {
  for (int i = spos; i < epos; i++) {
    (*res)[i] = 0.0;
    if (i == getTypeIndex()) {
      (*res)[i] = getTime();
    }
  }
}

void TaskTypeMetricHandler::updateValues(Array<double, NUM_UC_DOUBLE> *res,
                                         int spos, int epos, bool subtract) {
  int factor = subtract ? -1 : 1;
  for (int i = spos; i < epos; i++) {
    if ((i - spos) == getTypeIndex()) {
      (*res)[i] += factor * getTime();
    }
  }
}

// Initial: 0, Implicit (non-initial): 1, Explicit included: 2, Explicit
// deferred: 3, Target: 4
int TaskTypeMetricHandler::getTypeIndex() {
  if (taskTypeForDep & ompt_task_initial) {
    return 0;
  } else if (taskTypeForDep & ompt_task_implicit) {
    return 1;
  } else if (taskTypeForDep & ompt_task_undeferred) {
    return 2;
  } else if (taskTypeForDep & ompt_task_target) {
    return 4;
  } else {
    return 3;
  }
}

void TaskTypeMetricHandler::switchTaskType(int type) { taskTypeForDep = type; }

/*
 * Task prio metrics
 */

TaskPrioMetricHandler::TaskPrioMetricHandler() {
  description = "Task Prio Share on CP";
  numValues = 5;

  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Task prio metrics available.\n");
}

void TaskPrioMetricHandler::getValues(Array<double, NUM_UC_DOUBLE> *res,
                                      int spos, int epos) {
  DCHECK_NE(prios[0],
            -1); // we should track at least one prio, otherwise this function
                 // should not be called since prios are disabled
  for (int i = spos; i < epos; i++) {
    (*res)[i] = 0.0;
    if (prios[i - spos] == taskPrioForDep) {
      (*res)[i] = getTime();
    }
  }
}

void TaskPrioMetricHandler::updateValues(Array<double, NUM_UC_DOUBLE> *res,
                                         int spos, int epos, bool subtract) {
  DCHECK_NE(prios[0],
            -1); // we should track at least one prio, otherwise this function
                 // should not be called since prios are disabled
  int factor = subtract ? -1 : 1;
  for (int i = spos; i < epos; i++) {
    if (prios[i - spos] == taskPrioForDep) {
      (*res)[i] += factor * getTime();
    }
  }
}

void TaskPrioMetricHandler::addTaskPrios() {
  std::istringstream ss(analysis_flags->task_prio_shares);
  std::string token;
  int count = 0;
  while (std::getline(ss, token, ';')) {
    const char *nptr = token.c_str();
    char *end = NULL;
    int prio;
    errno = 0;

    prio = strtol(token.c_str(), &end, 10);
    if (errno != 0 || nptr == end) {
      fprintf(stderr,
              "Warning: Invalid argument for task priority: '%s'. Skipped.\n",
              token.c_str());
      continue;
    }
    if (prio >= 0) {
      if (count < numValues) {
        prios[count] = prio;
        count++;
      } else {
        fprintf(stderr,
                "Warning: Only %i task prios can be tracked at once. Skipping "
                "all additional prios.\n",
                numValues);
      }
    } else {
      fprintf(stderr,
              "Warning: Negative numbers are invalid for task priority: %i. "
              "Skipped.\n",
              prio);
    }
  }
}

void TaskPrioMetricHandler::switchTaskPrio(int prio) { taskPrioForDep = prio; }

/*
 * Task name metrics
 */

TaskNameMetricHandler::TaskNameMetricHandler() {
  description = "Task Name Share on CP";
  numValues = 5;

  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Task name metrics available.\n");
}

void TaskNameMetricHandler::getValues(Array<double, NUM_UC_DOUBLE> *res,
                                      int spos, int epos) {
  DCHECK_NOT(names[0].empty());
  for (int i = spos; i < epos; i++) {
    (*res)[i] = 0.0;
    // we do not want to track empty names
    if (names[i - spos].empty())
      continue;
    if (names[i - spos] == taskNameForDep) {
      (*res)[i] = getTime();
    }
  }
}

void TaskNameMetricHandler::updateValues(Array<double, NUM_UC_DOUBLE> *res,
                                         int spos, int epos, bool subtract) {
  DCHECK_NOT(names[0].empty());
  int factor = subtract ? -1 : 1;
  for (int i = spos; i < epos; i++) {
    // we do not want to track empty names
    if (names[i - spos].empty())
      continue;
    if (names[i - spos] == taskNameForDep) {
      (*res)[i] += factor * getTime();
    }
  }
}

void TaskNameMetricHandler::addTaskNames() {
  std::istringstream ss(analysis_flags->task_name_shares);
  std::string token;
  int count = 0;
  while (std::getline(ss, token, ';')) {
    if (count < numValues) {
      names[count] = token;
      count++;
    } else {
      printf("Warning: Only %i task names can be tracked at once. Skipping all "
             "additional names.\n",
             numValues);
    }
  }
}

void TaskNameMetricHandler::switchTaskName(std::string name) {
  taskNameForDep = name;
}

/*
 * Task count metrics
 */

TaskCountMetricHandler::TaskCountMetricHandler() {
  description = "Number of Tasks on CP";
  numValues = 1;

  if (analysis_flags->verbose)
    fprintf(analysis_flags->output, "Task count metrics available.\n");
}

void TaskCountMetricHandler::getValues(Array<uint64_t, NUM_UC_INT64> *res,
                                       int spos, int epos) {
  for (int i = spos; i < epos; i++) {
    (*res)[i] = taskCountForDep;
  }
}

void TaskCountMetricHandler::updateValues(Array<uint64_t, NUM_UC_INT64> *res,
                                          int spos, int epos, bool subtract) {
  int factor = subtract ? -1 : 1;
  for (int i = spos; i < epos; i++) {
    (*res)[i] += factor * taskCountForDep;
  }
}

void TaskCountMetricHandler::increaseTaskCount(uint64_t count) {
  taskCountForDep += count;
}

#endif // DEPENDENTMETRICS_C
