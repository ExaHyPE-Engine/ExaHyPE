#include "taskfusion.h"

#include <list>

#include "tarch/Assertions.h"

#include "tarch/multicore/Lock.h"

#include "tarch/logging/Statistics.h"

#include "tarch/logging/Log.h"

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


void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence(
  Task*                        task,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  ::tarch::logging::Statistics::getInstance().inc(SubmitFusedTaskStatisticsIdentifier, 1, false, true);

  assertion1(task->getTaskType()>=0, task->getTaskType());

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

  logDebug( "translateFusableTaskIntoTaskSequence(...)", "spawn two tasks around " << taskNumber );

  std::set<TaskNumber>  wrappedTaskNumber = {taskNumber};

  Task* readyTask   = new LogReadyTask(task,taskQueue);
  Task* processTask = new ProcessReadyTask(taskQueue);

  readyTask->setPriority(task->getPriority());
  processTask->setPriority(task->getPriority()/2);

  tarch::multicore::native::spawnTask( readyTask, inDependencies, taskNumber);
  tarch::multicore::native::spawnTask( processTask, wrappedTaskNumber, taskNumber);
}

