// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Task.h"

#include "BooleanSemaphore.h"
#include "Core.h"
#include "Lock.h"
#include "multicore.h"
#include "tarch/Assertions.h"
#include "tarch/services/ServiceRepository.h"

bool operator<(const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs) {
  return lhs.getPriority() < rhs.getPriority();
}

bool tarch::multicore::TaskComparison::operator()(const Task& lhs, const Task& rhs) const { return lhs < rhs; }

bool tarch::multicore::TaskComparison::operator()(Task* lhs, Task* rhs) const { return *lhs < *rhs; }

/**
 * Construct task
 *
 * ## Task types and task numbers
 *
 * Every task has a task type. All tasks which do the same work carry the same
 * task type marker. The tarch does not generate these markers: a caller
 * manages the numbering and hands the result in.
 *
 * Task numbers, which identify one individual task rather than a class of
 * tasks, are not part of this interface. They are required to express task
 * dependencies or to match the data another rank sends back for an offloaded
 * task against the task which would have produced these results locally. Not
 * every task needs them, though, and keeping unique numbers for all tasks
 * would be too expensive.
 *
 * @param taskType Unique task (type) number for this task.
 * @param priority Integer value that indicates which priority to assign to a
 *        task. Value has to be non-negative. Use DefaultPriority if there is
 *        no bespoke priority.
 */
tarch::multicore::Task::Task(int taskType, int priority):
  _taskType(taskType),
  _priority(priority) {
  assertion(priority >= 0, taskType, priority);
}

int tarch::multicore::Task::getPriority() const { return _priority; }

void tarch::multicore::Task::setPriority(int priority) {
  assertion(priority >= 0, _taskType, _priority, priority);
  _priority = priority;
}

int tarch::multicore::Task::getTaskType() const { return _taskType; }

std::string tarch::multicore::Task::toString() const { return "<no-description-available>"; }

tarch::multicore::TaskWithCopyOfFunctor::TaskWithCopyOfFunctor(
  int                          taskType,
  int                          priority,
  const std::function<bool()>& taskFunctor
#if PEANO_DEBUG
  ,
  const std::string& taskDescription
#endif
):
  Task(taskType, priority),
  _taskFunctor(taskFunctor)
#if PEANO_DEBUG
  ,
  _taskDescription(taskDescription)
#endif
{
}

bool tarch::multicore::TaskWithCopyOfFunctor::run() { return _taskFunctor(); }

std::string tarch::multicore::TaskWithCopyOfFunctor::toString() const {
#if PEANO_DEBUG
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
