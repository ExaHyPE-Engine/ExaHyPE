#include "ProcessSetOfReadyTasks.h"
#include "FusableTasksQueue.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log tarch::multicore::taskfusion::ProcessSetOfReadyTasks::_log( "tarch::multicore::taskfusion::ProcessSetOfReadyTasks" );
const std::string tarch::multicore::taskfusion::ProcessSetOfReadyTasks::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::process-fused-tasks";


tarch::multicore::taskfusion::ProcessSetOfReadyTasks::ProcessSetOfReadyTasks(
  TaskNumber          taskNumber,
  FusableTasksQueue&  taskQueue
):
  Task(DontFuse,DefaultPriority),
  _taskQueue( taskQueue ),
  _taskNumber( taskNumber ) {
  assertion( _taskNumber!= tarch::multicore::taskfusion::FusableTasksQueue::AnyTask );
}


void tarch::multicore::taskfusion::ProcessSetOfReadyTasks::run() {
  assertion( not _taskQueue.isTaskPending(_taskNumber, false) );

  if ( _taskQueue.isTaskReadyOrRunning(_taskNumber, FusableTasksQueue::LockQueueAndReturnReliableResult) ) {
    tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(_taskQueue.getType());
    assertion( instruction.minTasks>=1 );
    assertion( instruction.maxTasks>=1 );

    tarch::multicore::Lock lock(_taskQueue._statisticsSemaphore);
    _taskQueue._numberOfPendingReadyTasksPerProcessTaskExecution.setValue(_taskQueue._readyTasks.size());
    _taskQueue._probabilityThatTaskIsCompleted.setValue(0.0);

    if (_taskQueue._numberOfPendingReadyTasksPerProcessTaskExecution.hasFullSetOfMeasurements()) {
      tarch::multicore::getOrchestration().updateFuseStatistics(
        _taskQueue.getType(),
        _taskQueue._readyTasks.size(),
        _taskQueue._numberOfPendingReadyTasksPerProcessTaskExecution.getValue(),
        _taskQueue._probabilityThatTaskIsCompleted.getValue()
      );
    }
    lock.free();

    tarch::multicore::Lock readyTaskLock(_taskQueue._readyTasksSemaphore);
    if (
      instruction.minTasks<=_taskQueue._readyTasks.size()
    ) {
      // extract first task
      auto [firstTaskNumber, firstTask] = *_taskQueue._readyTasks.begin();
      _taskQueue._readyTasks.erase( firstTaskNumber );

      // extract sequence
      int tasksToExtract = instruction.maxTasks-1;
      FusableTasksQueue::ReadyTasks::iterator iterator = _taskQueue._readyTasks.begin();

        // @todo Vector would be better, as we know how many taskst hereare
        // @todo Can we grab all of them in one go if the set of ready tasks is small?
        std::list< Task* > extractedTaskPointers;
        std::list< int >   extractedTaskNumbers;


      tarch::multicore::Lock runningTaskLock(_taskQueue._runningTasksSemaphore);
      _taskQueue._runningTasks[ firstTaskNumber ] = _taskNumber;
      while (tasksToExtract>0 and iterator != _taskQueue._readyTasks.end()) {
        tasksToExtract--;
        extractedTaskNumbers.push_back( iterator->first );
        extractedTaskPointers.push_back( iterator->second );
        _taskQueue._runningTasks[ iterator->first ] = _taskNumber;
        iterator = _taskQueue._readyTasks.erase( iterator );
      }
      runningTaskLock.free();
      readyTaskLock.free();

      logDebug( "run(...)", "fuse " << (extractedTaskPointers.size()+1) << " fusable tasks as we have " << _taskQueue._readyTasks.size() << " tasks (embedded into native task " << _taskNumber << ")" );

      tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTaskNumbers.size());
      processFusedTask(firstTask, extractedTaskPointers, instruction.device);

      runningTaskLock.lock();
      _taskQueue._runningTasks.erase( firstTaskNumber );
      for (auto& taskNumber: extractedTaskNumbers) {
        _taskQueue._runningTasks.erase( taskNumber );
      }
      runningTaskLock.free();

      logDebug( "run(...)", "fused task " << _taskNumber << " finished" );
    }
  }
  else {
    tarch::multicore::Lock lock(_taskQueue._statisticsSemaphore);
    _taskQueue._probabilityThatTaskIsCompleted.setValue(1.0);
  }
}

