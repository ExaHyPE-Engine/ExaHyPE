#include "taskfusion.h"

#include <list>

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"

#include "tarch/logging/Statistics.h"
#include "tarch/logging/Log.h"

#include "tarch/la/ScalarOperations.h"

#include "LogReadyTask.h"
#include "ProcessReadyTask.h"
#include "FusableTasksQueue.h"


namespace {
  /**
   * We map task types onto lists of tasks
   */
  std::map<int, tarch::multicore::taskfusion::FusableTasksQueue*>     fusableTaskQueue;
  tarch::multicore::BooleanSemaphore                                  pendingTasksSemaphore;

  tarch::logging::Log _log("tarch::multicore::taskfusion");

  const std::string SubmitFusedTaskStatisticsIdentifier("tarch::multicore::taskfusion::submit-fused-tasks");

  // 56.1246s
//  constexpr tarch::multicore::taskfusion::FusionImplementation ImplementationVariant = tarch::multicore::taskfusion::FusionImplementation::DisableFusionAndProcessImmediately;
  //  Buggy in OpenMP
//  constexpr tarch::multicore::taskfusion::FusionImplementation ImplementationVariant = tarch::multicore::taskfusion::FusionImplementation::DisableFusionAndMapOntoNativeTask;
  constexpr tarch::multicore::taskfusion::FusionImplementation ImplementationVariant = tarch::multicore::taskfusion::FusionImplementation::LazyEvaluation;
// constexpr tarch::multicore::taskfusion::FusionImplementation ImplementationVariant = tarch::multicore::taskfusion::FusionImplementation::LazyConsumerTasks;
//  constexpr tarch::multicore::taskfusion::FusionImplementation ImplementationVariant = tarch::multicore::taskfusion::FusionImplementation::ProducerConsumerTaskPairs;
}



void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence(
  Task*                  task,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber
) {
  ::tarch::logging::Statistics::getInstance().inc(SubmitFusedTaskStatisticsIdentifier, 1, false, true);

  assertion1(task->getTaskType()>=0, task->getTaskType());

  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(task->getTaskType());

  assertion1(instruction.device==tarch::multicore::Task::Host or instruction.device>=0, instruction.toString());
  assertion1(instruction.minTasks>=0, instruction.toString());
  assertion1(instruction.maxTasks>instruction.minTasks, instruction.toString());

  if (ImplementationVariant==tarch::multicore::taskfusion::FusionImplementation::DisableFusionAndMapOntoNativeTask) {
    native::spawnTask(task, inDependencies, taskNumber);
  }
  else if (ImplementationVariant==tarch::multicore::taskfusion::FusionImplementation::DisableFusionAndProcessImmediately) {
    task->run();
    delete task;
  }
  else if (instruction.maxTasks<=1) {
    native::spawnTask(task, inDependencies, taskNumber);
  }
  else {
    // ensure the entry exists
    tarch::multicore::Lock lock(pendingTasksSemaphore);
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

    switch (ImplementationVariant) {
      case FusionImplementation::DisableFusionAndMapOntoNativeTask:
      case FusionImplementation::DisableFusionAndProcessImmediately:
        assertion(false);
        break;
      case FusionImplementation::LazyEvaluation:
      case FusionImplementation::LazyConsumerTasks:
        translateFusableTaskIntoTaskSequence_LazyEvaluation(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
      case FusionImplementation::ProducerConsumerTaskPairs:
        translateFusableTaskIntoTaskSequence_ProducerConsumerTaskPairs(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
    }
  }
}


void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence_LazyEvaluation(
  Task*                  task,
  FusableTasksQueue&     taskQueue,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber,
  const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
) {
  if ( taskNumber==tarch::multicore::NoOutDependencies or inDependencies.empty() ) {
    taskQueue.insertReadyTask(
      task,
      taskNumber
    );
  }
  else {
    Task* readyTask   = new LogReadyTask(task, taskNumber, taskQueue);
    readyTask->setPriority(task->getPriority());
    tarch::multicore::native::spawnTask( readyTask, inDependencies, taskNumber);
  }
}


void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence_ProducerConsumerTaskPairs(
  Task*                  task,
  FusableTasksQueue&     taskQueue,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber,
  const tarch::multicore::orchestration::Strategy::FuseInstruction&  instruction
) {
  logDebug(
    "translateFusableTaskIntoTaskSequence(...)",
    "spawn two tasks around " << taskNumber <<
    " (min-tasks=" << instruction.minTasks <<
    ", max-tasks=" << instruction.maxTasks <<
    ")"
  );

  std::set<TaskNumber>  processReadyTaskInDependencyNumber;

  if ( taskNumber==tarch::multicore::NoOutDependencies or inDependencies.empty() ) {
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

  Task* processTask = new ProcessReadyTask(taskNumber, taskQueue);
  processTask->setPriority(task->getPriority()/2);
  tarch::multicore::native::spawnTask( processTask, processReadyTaskInDependencyNumber, taskNumber);
}


void tarch::multicore::taskfusion::processAllReadyTasks() {
  tarch::multicore::Lock lock(pendingTasksSemaphore);
  for (auto& p: fusableTaskQueue) {
    p.second->drainReadyTasks();
  }
}


void tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks(const std::set<TaskNumber>& inDependencies) {
  switch (ImplementationVariant) {
    case FusionImplementation::ProducerConsumerTaskPairs:
    case FusionImplementation::DisableFusionAndProcessImmediately:
    case FusionImplementation::DisableFusionAndMapOntoNativeTask:
      break;
    case FusionImplementation::LazyEvaluation:
    case FusionImplementation::LazyConsumerTasks:
      ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation( inDependencies );
      break;
  }
}


void tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(const std::set<TaskNumber>& inDependencies) {
  bool hasBeenProcessed = true;

  for (auto taskNumber: inDependencies) {
    for (auto queue: fusableTaskQueue) {
      hasBeenProcessed &= not queue.second->hasTaskBeenProcessed(taskNumber, true);
    }
  }

  while (not hasBeenProcessed) {
    hasBeenProcessed = true;
    for (auto taskNumber: inDependencies) {
      for (auto queue: fusableTaskQueue) {
        if (not queue.second->hasTaskBeenProcessed(taskNumber, false) ) {
          queue.second->processOneTask(taskNumber);
        }
      }
    }
  }
}
