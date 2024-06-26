#include "Tasks.h"

#include <queue>
#include <set>
#include <thread>

#include "BooleanSemaphore.h"
#include "config.h"
#include "Core.h"
#include "Lock.h"
#include "multicore.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/orchestration/StrategyFactory.h"
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

  NonblockingTasks               globalNonblockingTasks;
  std::vector<NonblockingTasks*> localNonblockingTasks;

  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log("tarch::multicore");

  const std::string BSPConcurrencyLevelStatisticsIdentifier("tarch::multicore::bsp-concurrency-level");
  const std::string GlobalPendingTasksStatisticsIdentifier("tarch::multicore::global-pending-tasks");
  const std::string ThreadLocalPendingTasksStatisticsIdentifier("tarch::multicore::thread-local-pending-tasks");
  const std::string FuseTasksStatisticsIdentifier("tarch::multicore::fuse-tasks");

  /**
   * This routine processes one pending tasks.
   *
   * It has no hidden logic, i.e. it will always take one task
   * if possible. The routine calling it is reponsible to make
   * the decision whether this is appropriate.
   *
   * @return -1 if nothing found, otherwise task id
   */
  bool processOnePendingTaskLIFO() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(globalNonblockingTasks.semaphore);
    if (not globalNonblockingTasks.tasks.empty()) {
      myTask = globalNonblockingTasks.tasks.back();
      globalNonblockingTasks.tasks.pop_back();
    }
    lock.free();

    if (myTask != nullptr) {
      bool requeue = myTask->run();
      if (requeue) {
        spawnTask(myTask);
      } else {
        delete myTask;
      }
    }
    return myTask != nullptr;
  }

  bool processOnePendingTaskFIFO() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(globalNonblockingTasks.semaphore);
    if (not globalNonblockingTasks.tasks.empty()) {
      myTask = globalNonblockingTasks.tasks.front();
      globalNonblockingTasks.tasks.pop_front();
    }
    lock.free();

    if (myTask != nullptr) {
      bool requeue = myTask->run();
      if (requeue) {
        spawnTask(myTask);
      } else {
        delete myTask;
      }
      return true;
    } else {
      return false;
    }
  }
} // namespace


/**
 * @return Number of processed tasks
 */
int tarch::multicore::internal::mapPendingTasksOntoNativeTasks(int maxTasks) {
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
}

bool tarch::multicore::internal::fusePendingTasks(const tarch::multicore::orchestration::Strategy::FuseInstruction& fuseInstruction) {
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
      pp != globalNonblockingTasks.tasks.end() and (*pp)->getTaskType() == myTask->getTaskType() and /*(*pp)->getTaskType() == myTask->canFuse()*/ (*pp)->canFuse()
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
}

void tarch::multicore::setOrchestration(tarch::multicore::orchestration::Strategy* realisation) {
  assertion(orchestrationStrategy != nullptr);
  assertion(realisation != nullptr);

  delete orchestrationStrategy;
  orchestrationStrategy = realisation;
}

tarch::multicore::orchestration::Strategy* tarch::multicore::swapOrchestration(tarch::multicore::orchestration::Strategy* realisation) {
  assertion(orchestrationStrategy != nullptr);
  assertion(realisation != nullptr);

  tarch::multicore::orchestration::Strategy* result = orchestrationStrategy;
  orchestrationStrategy                             = realisation;
  return result;
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

  return result;
}


void tarch::multicore::spawnTask(
  Task*                        task,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  assertion(task != nullptr);

  if (taskNumber == NoOutDependencies and inDependencies.empty()) {
    int threadNumber = Core::getInstance().getThreadNumber();
    assertion1(threadNumber >= 0, threadNumber);
    assertion2(threadNumber < localNonblockingTasks.size(), threadNumber, localNonblockingTasks.size());

#ifdef UseSmartMPI
    if (task->isSmartMPITask() and smartmpi::spawn(dynamic_cast<smartmpi::Task*>(task))) {
    } else
#endif
      if (localNonblockingTasks[threadNumber]->tasks.size() >= orchestrationStrategy->getNumberOfTasksToHoldBack(task->getTaskType())) {
      logDebug("spawnTasks(int)", "spawn native task of type " << task->getTaskType());
      native::spawnTask(task, std::set<TaskNumber>(), NoOutDependencies);
    } else {
      tarch::multicore::Lock lock(localNonblockingTasks[threadNumber]->semaphore);
      localNonblockingTasks[threadNumber]->tasks.push_back(task);
      lock.free();

      ::tarch::logging::Statistics::getInstance().log(ThreadLocalPendingTasksStatisticsIdentifier, localNonblockingTasks[threadNumber]->tasks.size());

      logDebug("spawnTask(...)", "enqueued task (#tasks=" << localNonblockingTasks[threadNumber]->tasks.size() << ")");

      auto fusionCommand = orchestrationStrategy->getNumberOfTasksToFuseAndTargetDevice(task->getTaskType());
      if (localNonblockingTasks[threadNumber]->tasks.size() >= fusionCommand.minTasks and orchestrationStrategy->fuseTasksImmediatelyWhenSpawned(task->getTaskType())) {
        internal::copyInternalTaskQueueOverIntoGlobalQueue(threadNumber);
        internal::fusePendingTasks(fusionCommand);
      }
    }
  } else {
    tarch::multicore::native::spawnTask(task, inDependencies, taskNumber);
  }
}

void tarch::multicore::spawnAndWait(const std::vector<Task*>& tasks) {
  static tarch::logging::Log _log("tarch::multicore");

  if (not tasks.empty()) {
    static int                                nestedSpawnAndWaits = 0;
    static tarch::multicore::BooleanSemaphore nestingSemaphore;

    tarch::multicore::Lock nestingLock(nestingSemaphore, false);
    nestingLock.lock();
    nestedSpawnAndWaits++;
    const int localNestedSpawnAndWaits = nestedSpawnAndWaits;
    nestingLock.free();

    orchestrationStrategy->startBSPSection(localNestedSpawnAndWaits);

    switch (orchestrationStrategy->paralleliseForkJoinSection(localNestedSpawnAndWaits, tasks.size(), tasks[0]->getTaskType())) {
    case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially: {
      for (auto& p : tasks) {
        while (p->run()) {
          Core::getInstance().yield();
        }
        delete p;
      }
    } break;
    case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel: {
      ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
      native::spawnAndWaitAsTaskLoop(tasks);
      ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);

      internal::copyInternalTaskQueuesOverIntoGlobalQueue();
      int  numberOfPendingTasks = tarch::multicore::internal::getNumberOfWithholdPendingTasks();
      auto fusionInstruction    = orchestrationStrategy->getNumberOfTasksToFuseAndTargetDevice(tarch::multicore::orchestration::Strategy::EndOfBSPSection);

      bool successInFusing = true;
      while (successInFusing and globalNonblockingTasks.tasks.size() >= fusionInstruction.minTasks) {
        successInFusing = tarch::multicore::internal::fusePendingTasks(fusionInstruction);
      }

      if (not globalNonblockingTasks.tasks.empty()) {
        int numberOfTasksToProcessNow = std::max(
          0, static_cast<int>(globalNonblockingTasks.tasks.size()) - orchestrationStrategy->getNumberOfTasksToHoldBack(tarch::multicore::orchestration::Strategy::EndOfBSPSection)
        );
        internal::mapPendingTasksOntoNativeTasks(numberOfTasksToProcessNow);
      }
    } break;
    case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks: {
      ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
      native::spawnAndWaitAsTaskLoop(tasks);
      ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);

    } break;
    }

    orchestrationStrategy->endBSPSection(localNestedSpawnAndWaits);

    nestingLock.lock();
    nestedSpawnAndWaits--;
    nestingLock.free();
  }
}

void tarch::multicore::internal::configureInternalTaskQueues(int numberOfThreads) {
  while (localNonblockingTasks.size() < numberOfThreads) {
    localNonblockingTasks.push_back(new NonblockingTasks());
  }
}

void tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue() {
  for (int i = 0; i < localNonblockingTasks.size(); i++) {
    copyInternalTaskQueueOverIntoGlobalQueue(i);
  }
}

void tarch::multicore::internal::copyInternalTaskQueueOverIntoGlobalQueue(int threadNumber) {
  assertion(threadNumber >= 0);

  ::tarch::logging::Statistics::getInstance().log(ThreadLocalPendingTasksStatisticsIdentifier, localNonblockingTasks[threadNumber]->tasks.size());
  tarch::multicore::Lock globalLock(globalNonblockingTasks.semaphore);
  tarch::multicore::Lock localLock(localNonblockingTasks[threadNumber]->semaphore);
  std::copy(localNonblockingTasks[threadNumber]->tasks.begin(), localNonblockingTasks[threadNumber]->tasks.end(), std::back_inserter(globalNonblockingTasks.tasks));
  logDebug(
    "copyInternalTaskQueuesOverIntoGlobalQueue()",
    "added "
      << localNonblockingTasks[threadNumber]->tasks.size() << " tasks to global tasks. #global pending tasks=" << tarch::multicore::internal::getNumberOfWithholdPendingTasks()
  );
  localNonblockingTasks[threadNumber]->tasks.clear();
  ::tarch::logging::Statistics::getInstance().log(GlobalPendingTasksStatisticsIdentifier, tarch::multicore::internal::getNumberOfWithholdPendingTasks());
}

int tarch::multicore::internal::getNumberOfWithholdPendingTasks() { return globalNonblockingTasks.tasks.size(); }


void tarch::multicore::waitForAllTasks() {}


void tarch::multicore::waitForTasks(const std::set<TaskNumber>& inDependencies) { tarch::multicore::native::waitForTasks(inDependencies); }


void tarch::multicore::waitForTask(const int taskNumber) {
  std::set<int> tmp{taskNumber};
  waitForTasks(tmp);
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
