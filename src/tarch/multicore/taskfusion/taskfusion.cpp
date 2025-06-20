#include "taskfusion.h"

#include <list>

#include "tarch/Assertions.h"

#include "tarch/multicore/multicore.h"
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

  if (tarch::multicore::getOrchestration().getFusionImplementation()==tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask) {
    native::spawnTask(task, inDependencies, taskNumber);
  }
  else if (tarch::multicore::getOrchestration().getFusionImplementation()==tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately) {
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

    switch (tarch::multicore::getOrchestration().getFusionImplementation()) {
      case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask:
      case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately:
        assertion(false);
        break;
      case tarch::multicore::orchestration::FusionImplementation::LazyEvaluation:
      case tarch::multicore::orchestration::FusionImplementation::LazyConsumerTasks:
        translateFusableTaskIntoTaskSequence_LazyEvaluation(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
      case tarch::multicore::orchestration::FusionImplementation::EagerConsumerTasks:
        translateFusableTaskIntoTaskSequence_LazyEvaluation(task, *taskQueue, inDependencies, taskNumber, instruction);
        translateFusableTaskIntoTaskSequence_EagerConsumerTasks(task, *taskQueue, inDependencies, taskNumber, instruction);
        break;
      case tarch::multicore::orchestration::FusionImplementation::ProducerConsumerTaskPairs:
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


void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence_EagerConsumerTasks(
  Task*                  task,
  FusableTasksQueue&     taskQueue,
  std::set<TaskNumber>   inDependencies,
  const TaskNumber&      taskNumber,
  const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
) {
  if (taskQueue.readyTasks.size()>=fuseInstruction.minTasks) {
    Task* processTask = new ProcessReadyTask(taskNumber, taskQueue);
    tarch::multicore::native::spawnTask( processTask, tarch::multicore::NoInDependencies, taskNumber);
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
  switch (tarch::multicore::getOrchestration().getFusionImplementation()) {
    case tarch::multicore::orchestration::FusionImplementation::ProducerConsumerTaskPairs:
    case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately:
    case tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask:
      break;
    case tarch::multicore::orchestration::FusionImplementation::LazyEvaluation:
      ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation( inDependencies );
      break;
    case tarch::multicore::orchestration::FusionImplementation::LazyConsumerTasks:
    case tarch::multicore::orchestration::FusionImplementation::EagerConsumerTasks:
      ensureHeldBackTasksAreMappedOntoNativeTasks_LazyConsumerTasks( inDependencies );
      break;
  }
}


void tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(const std::set<TaskNumber>& inDependencies) {
  bool tasksArePending         = false;
  bool tasksAreReadyOrRunning  = false;

  for (auto taskNumber: inDependencies) {
    for (auto queue: fusableTaskQueue) {
      tasksArePending        |= queue.second->isTaskPending(taskNumber, false);
      tasksAreReadyOrRunning |= queue.second->isTaskReadyOrRunning(taskNumber, true);
    }
  }

  if (tasksArePending) {
    native::waitForTasks(inDependencies);
  }

  if (tasksAreReadyOrRunning) {
    for (auto taskNumber: inDependencies) {
      for (auto queue: fusableTaskQueue) {
        while (queue.second->isTaskReadyOrRunning(taskNumber, false)) {
          queue.second->processTasks();
          queue.second->processOneTask(taskNumber);
        }
      }
    }
  }
}


void tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_LazyConsumerTasks(const std::set<TaskNumber>& inDependencies) {
  bool tasksArePending         = false;

  for (auto taskNumber: inDependencies) {
    for (auto queue: fusableTaskQueue) {
      tasksArePending        |= queue.second->isTaskPending(taskNumber, false);
    }
  }

  if (tasksArePending) {
    native::waitForTasks(inDependencies);
  }

  for (auto taskNumber: inDependencies) {
    for (auto queue: fusableTaskQueue) {
      if (queue.second->isTaskReady(tarch::multicore::taskfusion::FusableTasksQueue::AnyTask, false) ) {
        Task* processTask = new ProcessReadyTask(taskNumber, *(queue.second));
        tarch::multicore::native::spawnTask( processTask, tarch::multicore::NoInDependencies, taskNumber);
      }
      while (queue.second->isTaskReadyOrRunning(taskNumber, false)) {
        queue.second->processOneTask(taskNumber);
      }
    }
  }
}
