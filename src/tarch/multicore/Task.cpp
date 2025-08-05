// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Task.h"

#include "BooleanSemaphore.h"
#include "Core.h"
#include "Lock.h"
#include "multicore.h"
#include "tarch/Assertions.h"
#include "tarch/services/ServiceRepository.h"

#include <queue>
#include <set>

bool operator<(const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs) {
  return lhs.getPriority() < rhs.getPriority();
}

bool tarch::multicore::TaskComparison::operator()(const Task& lhs, const Task& rhs) const { return lhs < rhs; }

bool tarch::multicore::TaskComparison::operator()(Task* lhs, Task* rhs) const { return *lhs < *rhs; }

tarch::multicore::Task::Task(int taskType, int priority):
  _taskType(taskType),
  _priority(priority) {
  assertion2(priority >= 0, taskType, priority);
}

int tarch::multicore::Task::getPriority() const { return _priority; }

void tarch::multicore::Task::setPriority(int priority) {
  assertion3(priority >= 0, _taskType, _priority, priority);
  _priority = priority;
}

int tarch::multicore::Task::getTaskType() const { return _taskType; }

void tarch::multicore::Task::fuse(const std::list<Task*>& otherTasks) {
  for (auto& otherTask: otherTasks) {
    while (otherTask->run()) {};
    delete otherTask;
  }
  while (this->run()) {};
}

std::string tarch::multicore::Task::toString() const { return "<no-description-available>"; }

tarch::multicore::TaskWithCopyOfFunctor::TaskWithCopyOfFunctor(
  int                          taskType,
  int                          priority,
  const std::function<bool()>& taskFunctor
#if PEANO_DEBUG > 0
  ,
  const std::string& taskDescription
#endif
):
  Task(taskType, priority),
  _taskFunctor(taskFunctor)
#if PEANO_DEBUG > 0
  ,
  _taskDescription(taskDescription)
#endif
{
}

bool tarch::multicore::TaskWithCopyOfFunctor::run() { return _taskFunctor(); }

std::string tarch::multicore::TaskWithCopyOfFunctor::toString() const {
#if PEANO_DEBUG > 0
  return _taskDescription;
#else
  return "";
#endif
}

tarch::multicore::TaskWithoutCopyOfFunctor::
  TaskWithoutCopyOfFunctor(int taskType, int priority, std::function<bool()>& taskFunctor):
  Task(taskType, priority),
  _taskFunctor(taskFunctor) {}

bool tarch::multicore::TaskWithoutCopyOfFunctor::run() { return _taskFunctor(); }

tarch::multicore::EmptyTask::EmptyTask(int priority):
  Task(Task::NoTaskType, priority) {}

bool tarch::multicore::EmptyTask::run() { return false; }

std::string tarch::multicore::EmptyTask::toString() const { return "empty"; }
