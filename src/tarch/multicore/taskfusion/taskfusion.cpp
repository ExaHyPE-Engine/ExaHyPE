#include "taskfusion.h"

#include <list>

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"

#include "tarch/logging/Statistics.h"
#include "tarch/logging/Log.h"

#include "tarch/la/ScalarOperations.h"

#include "LogReadyTask.h"
#include "ProcessReadyTask.h"


namespace {
  /**
   * We map task types onto lists of tasks
   */
  std::map<int, tarch::multicore::taskfusion::ReadyFusableTasks*>  pendingTasksThatCanBeFused;
  tarch::multicore::BooleanSemaphore                               pendingTasksSemaphore;

  tarch::logging::Log _log("tarch::multicore::taskfusion");

  const std::string SubmitFusedTaskStatisticsIdentifier("tarch::multicore::taskfusion::submit-fused-tasks");
}


tarch::multicore::taskfusion::ReadyFusableTasks::ReadyFusableTasks(int type_):
  type(type_),
  numberOfPendingReadyTasksPerProcessTaskExecution(),
  probabilityThatTaskIsCompleted(256)
{}


void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence(
  Task*                        task,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  ::tarch::logging::Statistics::getInstance().inc(SubmitFusedTaskStatisticsIdentifier, 1, false, true);

  assertion1(task->getTaskType()>=0, task->getTaskType());

  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(task->getTaskType());

  assertion1(instruction.device==tarch::multicore::Task::Host or instruction.device>=0, instruction.toString());
  assertion1(instruction.minTasks>=0, instruction.toString());
  assertion1(instruction.maxTasks>instruction.minTasks, instruction.toString());

  if (instruction.maxTasks<=1) {
    native::spawnTask(task, inDependencies, taskNumber);
  }
  else {
    // ensure the entry exists
    tarch::multicore::Lock lock(pendingTasksSemaphore);
    if (pendingTasksThatCanBeFused.count(task->getTaskType())==0) {
      pendingTasksThatCanBeFused.insert(
        std::pair<int, ReadyFusableTasks*>(
          task->getTaskType(),
          new ReadyFusableTasks(task->getTaskType())
        )
      );
    }
    ReadyFusableTasks* taskQueue = pendingTasksThatCanBeFused.at( task->getTaskType() );
    lock.free();

    logDebug(
      "translateFusableTaskIntoTaskSequence(...)",
      "spawn two tasks around " << taskNumber <<
      " (min-tasks=" << instruction.minTasks <<
      ", max-tasks=" << instruction.maxTasks <<
      ")"
    );

    std::set<TaskNumber>  processReadTaskInDependencyNumber;

    if ( taskNumber==tarch::multicore::NoOutDependencies or inDependencies.empty() ) {
      tarch::multicore::taskfusion::LogReadyTask::insertReadyTask(
        task,
        taskNumber,
        taskQueue
      );
    }
    else {
      Task* readyTask   = new LogReadyTask(task, taskNumber, taskQueue);
      readyTask->setPriority(task->getPriority());
      tarch::multicore::native::spawnTask( readyTask, inDependencies, taskNumber);
      processReadTaskInDependencyNumber.insert( taskNumber );
    }

    Task* processTask = new ProcessReadyTask(taskNumber, taskQueue);
    processTask->setPriority(task->getPriority()/2);
    tarch::multicore::native::spawnTask( processTask, processReadTaskInDependencyNumber, taskNumber);
  }
}


void tarch::multicore::taskfusion::processAllReadyTasks() {
  tarch::multicore::Lock lock(pendingTasksSemaphore);
  for (auto& p: pendingTasksThatCanBeFused) {
    const int AnyTask = -1;
    ProcessReadyTask::tryToProcessOneTask(p.second, AnyTask);
  }
}
