#include <cstdint>
#include <cstdio>
#ifndef DEPENDENTMETRICS_H
#define DEPENDENTMETRICS_H 1

#include "containers.h"
#include "cptConfig.h"
#include <string>

using namespace cpt;

class DepMetricHandler {
protected:
  // how many dependent metrics does this type create
  int numValues;
  // Description of the dependent metric type
  const char *description;

public:
  int getNumValues() { return numValues; }
  const char *getDescription() { return description; }
  virtual std::string getMetricName(int idx) = 0;
};

class IntDepMetricHandler : public DepMetricHandler {
public:
  // Get the current values and write them into positions spos until epos-1 from
  // res
  virtual void getValues(Array<uint64_t, NUM_UC_INT64> *res, int spos,
                         int epos) = 0;
  virtual void updateValues(Array<uint64_t, NUM_UC_INT64> *res, int spos,
                            int epos, bool subtract) = 0;
};

class DoubleDepMetricHandler : public DepMetricHandler {
public:
  // Get the current values and write them into positions spos until epos-1 from
  // res
  virtual void getValues(Array<double, NUM_UC_DOUBLE> *res, int startpos,
                         int endpos) = 0;
  virtual void updateValues(Array<double, NUM_UC_DOUBLE> *res, int startpos,
                            int endpos, bool subtract) = 0;
};

// Singleton class
// Keeps track of the different kinds of int and double dep metrics
class DepMetricOrganizer {
private:
  // needed to insure single instance
  static DepMetricOrganizer *instancePtr;
  static std::mutex mtx;

  DepMetricOrganizer() = default;
  DepMetricOrganizer(const DepMetricOrganizer &obj) = delete;

  int numIntValues = 0; // number of already reserved integer dependent values
  int numDoubleValues = 0; // number of already reserved double dependent values
  Vector<IntDepMetricHandler *> intDepMetricHandlers{};
  Vector<DoubleDepMetricHandler *> doubleDepMetricHandlers{};

public:
  static DepMetricOrganizer *getInstance() {
    if (instancePtr == nullptr) {
      std::lock_guard<std::mutex> lock(mtx);
      if (instancePtr == nullptr) {
        instancePtr = new DepMetricOrganizer();
      }
    }
    return instancePtr;
  }
  int getNumIntValues() { return numIntValues; }
  int getNumDoubleValues() { return numDoubleValues; }
  Vector<IntDepMetricHandler *> *getIntDepMetricHandlers() {
    return &intDepMetricHandlers;
  }
  Vector<DoubleDepMetricHandler *> *getDoubleDepMetricHandlers() {
    return &doubleDepMetricHandlers;
  }

  void registerIntMetricHandler(IntDepMetricHandler *handler) {
    if (numIntValues + handler->getNumValues() <= NUM_UC_INT64) {
      intDepMetricHandlers.PushBack(handler);
      numIntValues += handler->getNumValues();
      printf("Registered int dep metric handler '%s'\n",
             handler->getDescription());
    } else {
      printf("WARNING: Unable to add more integer dependent metrics! Skipping "
             "'%s'\n",
             handler->getDescription());
    }
  }
  void registerDoubleMetricHandler(DoubleDepMetricHandler *handler) {
    if (numDoubleValues + handler->getNumValues() <= NUM_UC_DOUBLE) {
      doubleDepMetricHandlers.PushBack(handler);
      numDoubleValues += handler->getNumValues();
      printf("Registered double dep metric handler '%s'\n",
             handler->getDescription());
    } else {
      printf("WARNING: Unable to add more double dependent metrics! Skipping "
             "'%s'\n",
             handler->getDescription());
    }
  }

  // writes the values from all different int dep metrics after another in the
  // provided array
  void getIntDepValues(Array<uint64_t, NUM_UC_INT64> *res) {
    int spos, epos = 0;
    for (int i = 0; i < intDepMetricHandlers.Size(); i++) {
      epos += intDepMetricHandlers[i]->getNumValues();
      DCHECK_LE(epos, NUM_UC_INT64);
      DCHECK_EQ(epos - spos, intDepMetricHandlers[i]->getNumValues());
      intDepMetricHandlers[i]->getValues(res, spos, epos);
      spos += intDepMetricHandlers[i]->getNumValues();
    }
  }
  template <bool negative>
  void updateIntDepValues(Array<uint64_t, NUM_UC_INT64> *res) {
    int spos = 0, epos = 0;
    for (int i = 0; i < intDepMetricHandlers.Size(); i++) {
      epos += intDepMetricHandlers[i]->getNumValues();
      DCHECK_LE(epos, NUM_UC_INT64);
      DCHECK_EQ(epos - spos, intDepMetricHandlers[i]->getNumValues());
      intDepMetricHandlers[i]->updateValues(res, spos, epos, negative);
      spos += intDepMetricHandlers[i]->getNumValues();
    }
  }
  // writes the values from all different double dep metrics after another in
  // the provided array
  void getDoubleDepValues(Array<double, NUM_UC_DOUBLE> *res) {
    int spos = 0, epos = 0;
    for (int i = 0; i < doubleDepMetricHandlers.Size(); i++) {
      epos += doubleDepMetricHandlers[i]->getNumValues();
      DCHECK_LE(epos, NUM_UC_DOUBLE);
      DCHECK_EQ(epos - spos, doubleDepMetricHandlers[i]->getNumValues());
      doubleDepMetricHandlers[i]->getValues(res, spos, epos);
      spos += doubleDepMetricHandlers[i]->getNumValues();
    }
  }
  template <bool negative>
  void updateDoubleDepValues(Array<double, NUM_UC_DOUBLE> *res) {
    int spos = 0, epos = 0;
    for (int i = 0; i < doubleDepMetricHandlers.Size(); i++) {
      epos += doubleDepMetricHandlers[i]->getNumValues();
      DCHECK_LE(epos, NUM_UC_DOUBLE);
      DCHECK_EQ(epos - spos, doubleDepMetricHandlers[i]->getNumValues());
      doubleDepMetricHandlers[i]->updateValues(res, spos, epos, negative);
      spos += doubleDepMetricHandlers[i]->getNumValues();
    }
  }
};

class TaskTypeMetricHandler : public DoubleDepMetricHandler {

private:
  // needed to insure single instance
  static TaskTypeMetricHandler *instancePtr;
  static std::mutex mtx;

  Array<std::string, 5> metricNames = {
      "Initial Task Share", "Implicit Task Share",
      "Explicit Included Task Share", "Explicit Deferred Task Share",
      "Target Task Share"};

  TaskTypeMetricHandler();
  TaskTypeMetricHandler(const TaskTypeMetricHandler &obj) = delete;

  int getTypeIndex();

public:
  static TaskTypeMetricHandler *getInstance() {
    if (instancePtr == nullptr) {
      std::lock_guard<std::mutex> lock(mtx);
      if (instancePtr == nullptr) {
        instancePtr = new TaskTypeMetricHandler();
      }
    }
    return instancePtr;
  }

  std::string getMetricName(int idx) {
    if (idx > numValues - 1 || idx < 0)
      return "";
    return metricNames[idx];
  }

  void getValues(Array<double, NUM_UC_DOUBLE> *res, int spos, int epos);
  void updateValues(Array<double, NUM_UC_DOUBLE> *res, int spos, int epos,
                    bool subtract);

  void switchTaskType(int type);
};

class TaskPrioMetricHandler : public DoubleDepMetricHandler {

private:
  // needed to insure single instance
  static TaskPrioMetricHandler *instancePtr;
  static std::mutex mtx;

  Array<int, 5> prios = {-1, -1, -1, -1, -1};

  TaskPrioMetricHandler();
  TaskPrioMetricHandler(const TaskPrioMetricHandler &obj) = delete;

public:
  static TaskPrioMetricHandler *getInstance() {
    if (instancePtr == nullptr) {
      std::lock_guard<std::mutex> lock(mtx);
      if (instancePtr == nullptr) {
        instancePtr = new TaskPrioMetricHandler();
      }
    }
    return instancePtr;
  }

  std::string getMetricName(int idx) {
    if (idx > numValues - 1 || idx < 0)
      return "";
    std::string prio = std::to_string(prios[idx]);

    return prio;
  }

  void getValues(Array<double, NUM_UC_DOUBLE> *res, int spos, int epos);
  void updateValues(Array<double, NUM_UC_DOUBLE> *res, int spos, int epos,
                    bool subtract);

  void addTaskPrios();
  void switchTaskPrio(int prio);
};

class TaskNameMetricHandler : public DoubleDepMetricHandler {

private:
  // needed to insure single instance
  static TaskNameMetricHandler *instancePtr;
  static std::mutex mtx;

  Array<std::string, 5> names = {};

  TaskNameMetricHandler();
  TaskNameMetricHandler(const TaskNameMetricHandler &obj) = delete;

public:
  static TaskNameMetricHandler *getInstance() {
    if (instancePtr == nullptr) {
      std::lock_guard<std::mutex> lock(mtx);
      if (instancePtr == nullptr) {
        instancePtr = new TaskNameMetricHandler();
      }
    }
    return instancePtr;
  }

  std::string getMetricName(int idx) {
    if (idx > numValues - 1 || idx < 0)
      return "";

    return names[idx];
  }

  void getValues(Array<double, NUM_UC_DOUBLE> *res, int spos, int epos);
  void updateValues(Array<double, NUM_UC_DOUBLE> *res, int spos, int epos,
                    bool subtract);

  void addTaskNames();
  void switchTaskName(std::string name);
};

class TaskCountMetricHandler : public IntDepMetricHandler {
private:
  // needed to insure single instance
  static TaskCountMetricHandler *instancePtr;
  static std::mutex mtx;

  TaskCountMetricHandler();
  TaskCountMetricHandler(const TaskCountMetricHandler &obj) = delete;

public:
  static TaskCountMetricHandler *getInstance() {
    if (instancePtr == nullptr) {
      std::lock_guard<std::mutex> lock(mtx);
      if (instancePtr == nullptr) {
        instancePtr = new TaskCountMetricHandler();
      }
    }
    return instancePtr;
  }

  std::string getMetricName(int idx) { return "NumTasks"; }

  void getValues(Array<uint64_t, NUM_UC_INT64> *res, int spos, int epos);
  void updateValues(Array<uint64_t, NUM_UC_INT64> *res, int spos, int epos,
                    bool subtract);

  void increaseTaskCount(uint64_t counts);
};

#endif // DEPENDENTMETRICS_H
