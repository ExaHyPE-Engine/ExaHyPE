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

namespace {
  /**
   * Task queue of tasks which we hold back
   */
  struct NonblockingTasks {
    NonblockingTasks()                        = default;
    ~NonblockingTasks()                       = default;
    NonblockingTasks(const NonblockingTasks&) = delete;

    typedef std::list<tarch::multicore::Task*> Tasks;
    Tasks                                      tasks;
    tarch::multicore::BooleanSemaphore         semaphore;
  };

  NonblockingTasks               pendingTasksThatCanBeFused;

  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log("tarch::multicore");

  // @todo Check if needed
  const std::string GlobalPendingTasksStatisticsIdentifier("tarch::multicore::global-pending-tasks");
  const std::string ThreadLocalPendingTasksStatisticsIdentifier("tarch::multicore::thread-local-pending-tasks");
  const std::string FuseTasksStatisticsIdentifier("tarch::multicore::fuse-tasks");
} // namespace


/**
 * @return Number of processed tasks
 */
int tarch::multicore::internal::mapPendingTasksOntoNativeTasks() {
/*
  NonblockingTasks::Tasks extractedTasks;

  tarch::multicore::Lock lock(globalNonblockingTasks.semaphore);
  maxTasks                                       = std::min(static_cast<int>(globalNonblockingTasks.tasks.size()), maxTasks);
  NonblockingTasks::Tasks::iterator cutIteration = globalNonblockingTasks.tasks.begin();
  std::advance(cutIteration, globalNonblockingTasks.tasks.size() - maxTasks);
  extractedTasks.splice(extractedTasks.begin(), globalNonblockingTasks.tasks, cutIteration, globalNonblockingTasks.tasks.end());
  lock.free();

  for (NonblockingTasks::Tasks::reverse_iterator task = extractedTasks.rbegin(); task != extractedTasks.rend(); task++) {
    tarch::multicore::native::spawnTask(*task, std::set<TaskNumber>(), NoOutDependencies);
  }

  return extractedTasks.size();
*/
}

bool tarch::multicore::internal::fusePendingTasks() {
/*
 * const tarch::multicore::orchestration::Strategy::FuseInstruction& fuseInstruction
  logDebug("fusePendingTasks(int)", "process " << globalNonblockingTasks.tasks.size() << " tasks subject to " << fuseInstruction.toString());

  ::tarch::logging::Statistics::getInstance().log(GlobalPendingTasksStatisticsIdentifier, tarch::multicore::internal::getNumberOfWithholdPendingTasks());

  tarch::multicore::Task*            myTask = nullptr;
  std::list<tarch::multicore::Task*> tasksOfSameType;

  tarch::multicore::Lock lock(globalNonblockingTasks.semaphore);
  if (not globalNonblockingTasks.tasks.empty()) {
    myTask = globalNonblockingTasks.tasks.front();
    globalNonblockingTasks.tasks.pop_front();
  }

  if (myTask != nullptr and myTask->canFuse()) {
    auto pp = globalNonblockingTasks.tasks.begin();
    while (
      pp != globalNonblockingTasks.tasks.end() and (*pp)->getTaskType() == myTask->getTaskType() and (*pp)->getTaskType() == myTask->canFuse() (*pp)->canFuse()
      and tasksOfSameType.size() < fuseInstruction.maxTasks - 1
    ) {
      tasksOfSameType.push_back(*pp);
      pp = globalNonblockingTasks.tasks.erase(pp);
    }
    lock.free();

    ::tarch::logging::Statistics::getInstance().log(FuseTasksStatisticsIdentifier, tasksOfSameType.size());

    native::processFusedTask(myTask, tasksOfSameType, fuseInstruction.device);
  } else if (myTask != nullptr and not myTask->canFuse()) {
    lock.free();
    tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoOutDependencies);
  }

  return myTask == nullptr ? 0 : tasksOfSameType.size() + 1;
*/
}

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

bool tarch::multicore::processPendingTasks(int maxTasks, bool fifo) {
  assertion(maxTasks >= 0);

  bool result = false;
/*
  while (maxTasks > 0) {
    if (fifo) {
      maxTasks--;
      result |= processOnePendingTaskFIFO();
    } else {
      maxTasks--;
      result |= processOnePendingTaskLIFO();
    }

    if (not result and maxTasks == 0) {
      internal::copyInternalTaskQueuesOverIntoGlobalQueue();
    }
  }
*/

  return result;
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


void tarch::multicore::native::waitForAllTasks() { logDebug("waitForAllTasks()", "wait"); }

#endif
