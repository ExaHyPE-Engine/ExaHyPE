#include "ProcessSetOfReadyTasks.h"
#include "taskfusion.h"

#include <list>

#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/MultiReadSingleWriteLock.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"

#include "tarch/logging/Statistics.h"
#include "tarch/logging/Log.h"

#include "tarch/la/ScalarOperations.h"

#include "LogReadyTask.h"
#include "FusableTasksQueue.h"
#include "ProcessSetOfReadyTasks.h"
#include "ProcessOneReadyTask.h"



namespace {
  /**
   * Map of task types onto queues
   *
   * See discussion in namespace documentation tarch::multicore::taskfusion
   * about this map and its thread access protection.
   */
  std::map<int, tarch::multicore::taskfusion::FusableTasksQueue*>     fusableTaskQueue;

  /**
   * Semaphore around fusableTaskQueue
   *
   * See discussion in namespace documentation tarch::multicore::taskfusion
   * about this map and its thread access protection.
   */
  tarch::multicore::MultiReadSingleWriteSemaphore  fusableTaskQueueMapSemaphore;

  tarch::logging::Log _log("tarch::multicore::taskfusion");
}


const std::string tarch::multicore::taskfusion::DirectlyInsertFusedTaskIntoReadyQueueStatisticsIdentifier("tarch::multicore::taskfusion::DirectlyInsertFusedTaskIntoReadyQueueStatisticsIdentifier");
const std::string tarch::multicore::taskfusion::MapFusedTaskOntoNativeTaskStatisticsIdentifier("tarch::multicore::taskfusion::MapFusedTaskOntoNativeTaskStatisticsIdentifier");
const std::string tarch::multicore::taskfusion::ProcessFusedTaskDirectlyStatisticsIdentifier("tarch::multicore::taskfusion::ProcessFusedTaskDirectlyStatisticsIdentifier");



void tarch::multicore::taskfusion::handleFusableTask(
  Task*                  task,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber
) {
  logDebug( "handleFusableTask(...)", "handle fusable task " << taskNumber << " with " << inDependencies.size() << " in-dependencies" );

  assertion1(task->getTaskType()>=0, task->getTaskType());

  if ( not inDependencies.empty() ) {
    taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks(inDependencies);
  }

  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(task->getTaskType());

  assertion1(instruction.device==tarch::multicore::Task::Host or instruction.device>=0, instruction.toString());
  assertion1(instruction.minTasks>=0, instruction.toString());
  assertion1(instruction.maxTasks>=instruction.minTasks, instruction.toString());

  if (tarch::multicore::getOrchestration().getFusionImplementation()==tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask) {
    logDebug( "handleFusableTask(...)", "map fusable task " << taskNumber << " onto native task" );
    ::tarch::logging::Statistics::getInstance().inc(MapFusedTaskOntoNativeTaskStatisticsIdentifier, 1, false, true);
    native::spawnTask(task, inDependencies, taskNumber);
  }
  else if (tarch::multicore::getOrchestration().getFusionImplementation()==tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately) {
    logDebug( "handleFusableTask(...)", "execute fusable task " << taskNumber << " directly" );
    ::tarch::logging::Statistics::getInstance().inc(ProcessFusedTaskDirectlyStatisticsIdentifier, 1, false, true);
    waitForTasks(inDependencies);
    task->run();
    delete task;
  }
  else if (instruction.maxTasks<=1) {
    logDebug( "handleFusableTask(...)", "map fusable task " << taskNumber << " onto native task" );
    ::tarch::logging::Statistics::getInstance().inc(MapFusedTaskOntoNativeTaskStatisticsIdentifier, 1, false, true);
    native::spawnTask(task, inDependencies, taskNumber);
  }
  else {
    tarch::multicore::MultiReadSingleWriteSemaphore::Lock lock(fusableTaskQueueMapSemaphore, ::tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Write);
    if (fusableTaskQueue.count(task->getTaskType())==0) {
      fusableTaskQueue.insert(
        std::pair<int, tarch::multicore::taskfusion::FusableTasksQueue*>(
          task->getTaskType(),
          new tarch::multicore::taskfusion::FusableTasksQueue(task->getTaskType())
        )
      );
    }
    FusableTasksQueue* taskQueue = fusableTaskQueue.at( task->getTaskType() );
    lock.free();

    switch (tarch::multicore::getOrchestration().getFusionImplementation()) {
      case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask:
      case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately:
        assertion(false);
        break;
      case tarch::multicore::orchestration::FusionImplementation::LazyEvaluation:
      case tarch::multicore::orchestration::FusionImplementation::TimedExecution:
        logDebug( "handleFusableTask(...)", "handle fusable task " << taskNumber << " over to fusion with lazy evaluation" );
        handleFusableTask_LazyEvaluation(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
      case tarch::multicore::orchestration::FusionImplementation::EagerConsumerTasks:
        logDebug( "handleFusableTask(...)", "handle fusable task " << taskNumber << " over to fusion with eager consumer tasks" );
        handleFusableTask_LazyEvaluation(task, *taskQueue, inDependencies, taskNumber, instruction);
        handleFusableTask_EagerConsumerTasks(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
      case tarch::multicore::orchestration::FusionImplementation::ProducerConsumerTaskSequence:
        logDebug( "handleFusableTask(...)", "handle fusable task " << taskNumber << " over to fusion with producer-consumer task sequence" );
        handleFusableTask_ProducerConsumerTaskSequence(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
    }
  }
}


void tarch::multicore::taskfusion::handleFusableTask_LazyEvaluation(
  Task*                  task,
  FusableTasksQueue&     taskQueue,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber,
  const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
) {
  logTraceInWith1Argument( "handleFusableTask_LazyEvaluation(...)", taskNumber );
  if ( taskNumber==tarch::multicore::NoOutDependencies or inDependencies.empty() ) {
    logDebug( "handleFusableTask_LazyEvaluation(...)", "insert task " << taskNumber << " directly into fusion queue" );
    ::tarch::logging::Statistics::getInstance().inc(DirectlyInsertFusedTaskIntoReadyQueueStatisticsIdentifier, 1, false, true);
    taskQueue.insertReadyTask(
      task,
      taskNumber
    );
  }
  else {
    logDebug( "handleFusableTask_LazyEvaluation(...)", "insert a ready task wrapping actual task " << taskNumber );
    Task* readyTask   = new LogReadyTask(task, taskNumber, taskQueue);
    readyTask->setPriority(task->getPriority());
    tarch::multicore::native::spawnTask( readyTask, inDependencies, taskNumber);
  }
  logTraceOutWith1Argument( "handleFusableTask_LazyEvaluation(...)", taskNumber );
}


void tarch::multicore::taskfusion::handleFusableTask_EagerConsumerTasks(
  Task*                  task,
  FusableTasksQueue&     taskQueue,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber,
  const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
) {
  if (taskQueue.getNumberOfReadyTasks()>=fuseInstruction.minTasks) {
    tarch::multicore::Task* newFusionTask = new tarch::multicore::taskfusion::ProcessSetOfReadyTasks(taskNumber, taskQueue);
    tarch::multicore::spawnTask(
      newFusionTask,
      tarch::multicore::NoInDependencies,
      taskNumber
    );
  }
}


void tarch::multicore::taskfusion::handleFusableTask_ProducerConsumerTaskSequence(
  Task*                  task,
  FusableTasksQueue&     taskQueue,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber,
  const tarch::multicore::orchestration::Strategy::FuseInstruction&  instruction
) {
  logDebug(
    "handleFusableTask(...)",
    "spawn three tasks around " << taskNumber <<
    " (min-tasks=" << instruction.minTasks <<
    ", max-tasks=" << instruction.maxTasks <<
    ")"
  );

  std::set<TaskNumber>  processReadyTaskInDependencyNumber = tarch::multicore::NoInDependencies;

  if ( taskNumber==tarch::multicore::NoOutDependencies or inDependencies.empty() ) {
    ::tarch::logging::Statistics::getInstance().inc(DirectlyInsertFusedTaskIntoReadyQueueStatisticsIdentifier, 1, false, true);
    taskQueue.insertReadyTask(
      task,
      taskNumber
    );
  }
  else {
    Task* readyTask   = new LogReadyTask(task, taskNumber, taskQueue);
    readyTask->setPriority(task->getPriority());
    tarch::multicore::native::spawnTask( readyTask, inDependencies, taskNumber);
    processReadyTaskInDependencyNumber.insert( taskNumber );
  }

  tarch::multicore::Task* newFusionTask = new tarch::multicore::taskfusion::ProcessSetOfReadyTasks(taskNumber, taskQueue);
  tarch::multicore::spawnTask(
    newFusionTask,
    processReadyTaskInDependencyNumber,
    taskNumber
  );

  tarch::multicore::Task* newProcessTask = new tarch::multicore::taskfusion::ProcessOneReadyTask(taskNumber, taskQueue);
  tarch::multicore::spawnTask(
    newProcessTask,
    {taskNumber},
    taskNumber
  );
}


void tarch::multicore::taskfusion::processAllReadyTasks() {
  tarch::multicore::MultiReadSingleWriteSemaphore::Lock lock(fusableTaskQueueMapSemaphore, ::tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);
  for (auto& p: fusableTaskQueue) {
    p.second->drainReadyTasks();
  }
}


void tarch::multicore::taskfusion::processAndFuseAllReadyTasks() {
  tarch::multicore::TaskNumber pendingReadyTask = tarch::multicore::NoOutDependencies + 1;
  while (pendingReadyTask != tarch::multicore::NoOutDependencies) {
    pendingReadyTask = tarch::multicore::NoOutDependencies;

    tarch::multicore::MultiReadSingleWriteSemaphore::Lock lock(fusableTaskQueueMapSemaphore, ::tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);
    for (auto& p: fusableTaskQueue) {
      if (pendingReadyTask!=tarch::multicore::NoOutDependencies) {
        pendingReadyTask = p.second->getFirstReadyTaskNumber();
      }
    }
    lock.free();

    if (pendingReadyTask!=tarch::multicore::NoOutDependencies) {
      waitForTask(pendingReadyTask);
    }
  }
}


bool tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks(const std::set<TaskNumber>& taskNumbers) {
  assertion1( not taskNumbers.empty(), taskNumbers.size() );
  logDebug( "ensureHeldBackTasksAreMappedOntoNativeTasks(...)", "wait for tasks " << toString(taskNumbers) );
  switch (tarch::multicore::getOrchestration().getFusionImplementation()) {
    case tarch::multicore::orchestration::FusionImplementation::ProducerConsumerTaskSequence:
      return true;
    case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately:
      return false;
    case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask:
      return true;
    case tarch::multicore::orchestration::FusionImplementation::LazyEvaluation:
    case tarch::multicore::orchestration::FusionImplementation::TimedExecution:
      return ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation( taskNumbers );
    case tarch::multicore::orchestration::FusionImplementation::EagerConsumerTasks:
      return ensureHeldBackTasksAreMappedOntoNativeTasks_EagerConsumerTasks( taskNumbers );
  }

  return true;
}


bool anyTasksOfInDendenciesMightBeStillReadyOrRunning(const std::set<tarch::multicore::TaskNumber>& inDependencies) {
  logTraceInWith1Argument( "anyTasksOfInDendenciesMightBeStillReadyOrRunning(std::set)", inDependencies.size() );
  bool tasksAreReadyOrRunning  = false;

  for (auto taskNumber: inDependencies) {
    for (auto queue: fusableTaskQueue) {
      tasksAreReadyOrRunning |= queue.second->isTaskReadyOrRunning(taskNumber, true);
    }
  }

  logTraceOutWith1Argument( "anyTasksOfInDendenciesMightBeStillReadyOrRunning(std::set)", tasksAreReadyOrRunning );
  return tasksAreReadyOrRunning;
}


bool tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_EagerConsumerTasks(const std::set<TaskNumber>& inDependencies) {
  logTraceInWith1Argument( "ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(std::set)", inDependencies.size() );

  assertion1( not inDependencies.empty(), inDependencies.size() );

  native::waitForTasks(inDependencies);

  bool result = false;

  if (anyTasksOfInDendenciesMightBeStillReadyOrRunning(inDependencies)) {
    for (auto taskNumber: inDependencies) {
      for (auto queue: fusableTaskQueue) {
        if (queue.second->isTaskReadyOrRunning(taskNumber, FusableTasksQueue::LockQueueAndReturnReliableResult)) {
          tarch::multicore::Task* newProcessTask = new tarch::multicore::taskfusion::ProcessOneReadyTask(taskNumber, *(queue.second));
          tarch::multicore::spawnTask(
            newProcessTask,
            {taskNumber},
            taskNumber
          );

          result = true;
        }
      }
    }
  }

  logTraceOutWith1Argument( "ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(std::set)", result );

  return result;
}


bool tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(const std::set<TaskNumber>& inDependencies) {
  logTraceInWith1Argument( "ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(std::set)", toString(inDependencies) );

  assertion1( not inDependencies.empty(), inDependencies.size() );

  native::waitForTasks(inDependencies);

  bool result = false;

  if (anyTasksOfInDendenciesMightBeStillReadyOrRunning(inDependencies)) {
    for (auto taskNumber: inDependencies) {
      for (auto queue: fusableTaskQueue) {
        if (queue.second->isTaskReadyOrRunning(taskNumber, FusableTasksQueue::LockQueueAndReturnReliableResult)) {
          logDebug( "ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(std::set)", "fusable task " << taskNumber << " sits in queue and is ready or already running" );
          tarch::multicore::Task* newFusionTask = new tarch::multicore::taskfusion::ProcessSetOfReadyTasks(taskNumber, *(queue.second));
          tarch::multicore::spawnTask(
            newFusionTask,
            tarch::multicore::NoInDependencies,
            taskNumber
          );

          tarch::multicore::Task* newProcessTask = new tarch::multicore::taskfusion::ProcessOneReadyTask(taskNumber, *(queue.second));
          tarch::multicore::spawnTask(
            newProcessTask,
            {taskNumber},
            taskNumber
          );

          logDebug( "ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(...)", "issued two process tasks " << taskNumber << "->" << taskNumber );

          result = true;
        }
      }
    }
  }

  logTraceOutWith1Argument( "ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(std::set)", result );
  return result;
}

