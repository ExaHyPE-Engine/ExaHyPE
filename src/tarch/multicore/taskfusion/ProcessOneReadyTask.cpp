#include "FusableTasksQueue.h"
#include "ProcessOneReadyTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log tarch::multicore::taskfusion::ProcessOneReadyTask::_log( "tarch::multicore::taskfusion::ProcessOneReadyTask" );


tarch::multicore::taskfusion::ProcessOneReadyTask::ProcessOneReadyTask(
  TaskNumber          taskNumber,
  FusableTasksQueue&  taskQueue
):
  Task(DontFuse,DefaultPriority),
  _taskQueue(taskQueue),
  _taskNumber(taskNumber) {
  assertion( _taskNumber!=tarch::multicore::taskfusion::FusableTasksQueue::AnyTask );
}


void tarch::multicore::taskfusion::ProcessOneReadyTask::run() {
  if ( _taskQueue.isTaskReadyOrRunning(_taskNumber, FusableTasksQueue::LockQueueAndReturnReliableResult) ) {
    logInfo( "run()", "task " << _taskNumber << " might be ready or running. Check again" );

    // register myself as the owner of this task. Should not be required, as I'd never wait for myself, but anyway
    Task* task =  _taskQueue.extractReadyTask( _taskNumber, _taskNumber );

    if ( task!=nullptr ) {
      logDebug( "run()", "task " << _taskNumber << " is ready, i.e. not running yet. Execute it manually" );

      task->run();

      delete task;

      _taskQueue.runningTaskFinished(_taskNumber);
    }
    else {
      logDebug( "run()", "task " << _taskNumber << " seems to be running atm (grabbed by another consumer task)" );

      TaskNumber taskNumberOfCurrentlyRunningFusion = _taskQueue.getTaskHandlingRunningTask( _taskNumber );

      if (taskNumberOfCurrentlyRunningFusion!=NoOutDependencies) {
        logDebug( "run(...)", "task " << _taskNumber << " has to wait for task " << taskNumberOfCurrentlyRunningFusion << " which seems to have grabbed its fusable task" );
        native::waitForTasks({taskNumberOfCurrentlyRunningFusion});
      }
    }
  }
  else {
    _taskQueue.updateTaskFusionProbabilities(false);
  }
}


