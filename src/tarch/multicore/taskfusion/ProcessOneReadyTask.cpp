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
  assertion( not _taskQueue.isTaskPending(_taskNumber, false) );

  if ( _taskQueue.isTaskReadyOrRunning(_taskNumber, FusableTasksQueue::LockQueueAndReturnReliableResult) ) {
    tarch::multicore::Lock readyTaskLock(_taskQueue._readyTasksSemaphore);
    if ( _taskQueue._readyTasks.count(_taskNumber)>0 ) {
      tarch::multicore::Lock runningTaskLock(_taskQueue._runningTasksSemaphore);

      _taskQueue._runningTasks[ _taskNumber ] = _taskNumber; // register myself as the owner of this task. Should not be required, as I'd never wait for myself, but anyway
      Task* task = _taskQueue._readyTasks[_taskNumber];
      _taskQueue._readyTasks.erase(_taskNumber);

      readyTaskLock.free();
      runningTaskLock.free();

      task->run();

      runningTaskLock.lock();
      _taskQueue._runningTasks.erase( _taskNumber );
      runningTaskLock.free();

      delete task;
    }
    else {
      readyTaskLock.free();

      TaskNumber taskNumberOfCurrentlyRunningFusion = NoOutDependencies;
      tarch::multicore::Lock runningTaskLock(_taskQueue._readyTasksSemaphore);
      if (_taskQueue._runningTasks.count(_taskNumber)) {
        taskNumberOfCurrentlyRunningFusion = _taskQueue._runningTasks[_taskNumber];
      }
      runningTaskLock.free();

      if (taskNumberOfCurrentlyRunningFusion!=NoOutDependencies) {
        logDebug( "run(...)", "task " << _taskNumber << " has to wait for task " << taskNumberOfCurrentlyRunningFusion << " which seems to have grabbed its fusable task" );
        native::waitForTasks({taskNumberOfCurrentlyRunningFusion});
      }
    }
  }
  else {
    tarch::multicore::Lock lock(_taskQueue._statisticsSemaphore);
    _taskQueue._probabilityThatTaskIsCompleted.setValue(1.0);
  }
}


