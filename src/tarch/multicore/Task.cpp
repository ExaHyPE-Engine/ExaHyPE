#include "Task.h"

#include <queue>
#include <set>
#include <thread>

#include "BooleanSemaphore.h"
#include "config.h"
#include "Core.h"
#include "Lock.h"
#include "multicore.h"
#include "tarch/Assertions.h"
#include "tarch/services/ServiceRepository.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#endif


bool operator<(const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs) { return lhs.getPriority() < rhs.getPriority(); }

bool tarch::multicore::TaskComparison::operator()(const Task& lhs, const Task& rhs) const { return lhs < rhs; }

bool tarch::multicore::TaskComparison::operator()(Task* lhs, Task* rhs) const { return *lhs < *rhs; }

tarch::multicore::Task::Task(int taskType, int priority):
  _taskType(taskType),
  _priority(priority) {
  assertion2(priority >= 0, taskType, priority);
}

bool tarch::multicore::Task::canFuse() const { return _taskType != DontFuse; }

int tarch::multicore::Task::getPriority() const { return _priority; }

void tarch::multicore::Task::setPriority(int priority) {
  assertion3(priority >= 0, _taskType, _priority, priority);
  _priority = priority;
}

int tarch::multicore::Task::getTaskType() const { return _taskType; }

bool tarch::multicore::Task::fuse(const std::list<Task*>& otherTasks, int device) {
  assertion(canFuse());
  for (auto pp : otherTasks) {
    tarch::multicore::Task* currentTask = pp;
    while (currentTask->run()) {
    }
    delete currentTask;
  }
  return true;
}

tarch::multicore::TaskWithCopyOfFunctor::TaskWithCopyOfFunctor(int taskType, int priority, const std::function<bool()>& taskFunctor):
  Task(taskType, priority),
  _taskFunctor(taskFunctor) {
}

bool tarch::multicore::TaskWithCopyOfFunctor::run() {
  return _taskFunctor();
}

tarch::multicore::TaskWithoutCopyOfFunctor::TaskWithoutCopyOfFunctor(int taskType, int priority, std::function<bool()>& taskFunctor):
  Task(taskType, priority),
  _taskFunctor(taskFunctor) {
}

bool tarch::multicore::TaskWithoutCopyOfFunctor::run() {
  return _taskFunctor();
}


#ifndef SharedMemoryParallelisation

#include <thread>


void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<Task*>& tasks) {
  for (auto& p : tasks) {
    while (p->run()) {
    }
    delete p;
  }
}

void tarch::multicore::native::processFusedTask(Task* myTask, const std::list<tarch::multicore::Task*>& tasksOfSameType, int device) {
  bool stillExecuteLocally = myTask->fuse(tasksOfSameType, device);
  if (stillExecuteLocally) {
    tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoOutDependencies);
  } else {
    delete myTask;
  }
}


void tarch::multicore::native::spawnTask(
  Task* job,
  const std::set<TaskNumber>& inDependencies,
  const TaskNumber& taskNumber
) {
  while (job->run()) {
  }
  delete job;
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {}


void tarch::multicore::native::waitForAllTasks() {}

#endif
