#include "ProcessReadyTask.h"
#include "FusableTasksQueue.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log tarch::multicore::taskfusion::ProcessReadyTask::_log( "tarch::multicore::taskfusion::ProcessReadyTask" );


tarch::multicore::taskfusion::ProcessReadyTask::ProcessReadyTask( int taskNumber, FusableTasksQueue& _taskQueue ):
  Task(DontFuse,DefaultPriority),
  _taskNumber( taskNumber ),
  _taskQueue(_taskQueue) {
}


void tarch::multicore::taskfusion::ProcessReadyTask::run() {
  assertion( not _taskQueue.isTaskPending(_taskNumber, false) );

  const bool isTaskStillPending = _taskQueue.isTaskReadyOrRunning(_taskNumber, true);

  if ( isTaskStillPending ) {
    tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(_taskQueue.getType());

    tarch::multicore::Lock lock(_taskQueue.statisticsSemaphore);
    _taskQueue.numberOfPendingReadyTasksPerProcessTaskExecution.setValue(_taskQueue.readyTasks.size());
    _taskQueue.probabilityThatTaskIsCompleted.setValue(0.0);

    if (_taskQueue.numberOfPendingReadyTasksPerProcessTaskExecution.hasFullSetOfMeasurements()) {
      tarch::multicore::getOrchestration().updateFuseStatistics(
        _taskQueue.getType(),
        _taskQueue.readyTasks.size(),
        _taskQueue.numberOfPendingReadyTasksPerProcessTaskExecution.getValue(),
        _taskQueue.probabilityThatTaskIsCompleted.getValue()
      );
    }
    lock.free();

    if (
      instruction.minTasks<=_taskQueue.readyTasks.size()
    ) {
      logDebug( "run(...)", "fuse as we have " << _taskQueue.readyTasks.size() << " tasks" );
      _taskQueue.processTasks();
    }

    _taskQueue.processOneTask(_taskNumber);
  }
  else {
    tarch::multicore::Lock lock(_taskQueue.statisticsSemaphore);
    _taskQueue.probabilityThatTaskIsCompleted.setValue(1.0);
  }
}


