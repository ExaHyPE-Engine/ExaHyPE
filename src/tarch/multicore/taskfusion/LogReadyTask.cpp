#include "LogReadyTask.h"

#include "tarch/multicore/Lock.h"

#include "tarch/logging/Statistics.h"


const std::string tarch::multicore::taskfusion::LogReadyTask::SubmitFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::submit-fused-tasks";
tarch::logging::Log tarch::multicore::taskfusion::LogReadyTask::_log( "tarch::multicore::taskfusion::LogReadyTask" );


tarch::multicore::taskfusion::LogReadyTask::LogReadyTask( Task* task, int taskNumber, ReadyFusableTasks* taskQueue ):
  Task(DontFuse,DefaultPriority),
  _task(task),
  _taskNumber(taskNumber),
  _taskQueue(taskQueue) {
}


void tarch::multicore::taskfusion::LogReadyTask::run() {
  ::tarch::logging::Statistics::getInstance().inc(SubmitFusedTasksStatisticsIdentifier, 1, false, true);

  logDebug( "run(...)", "task of type " << _task->getTaskType() << " is now ready to run. Put into ready queue" );

  insertReadyTask(_task, _taskNumber, _taskQueue);
}


void tarch::multicore::taskfusion::LogReadyTask::insertReadyTask( Task* task, int taskNumber, ReadyFusableTasks* taskQueue ) {
  tarch::multicore::Lock completedTasksLock(taskQueue->completedTasksSemaphore);
  ReadyFusableTasks::FinishedTaskNumbers::iterator p = taskQueue->completedTaskNumbers.find( taskNumber );
  if ( p != taskQueue->completedTaskNumbers.end() ) {
    taskQueue->completedTaskNumbers.erase(p);
  }


  tarch::multicore::Lock readyTasksLock(taskQueue->readyTasksSemaphore);
  taskQueue->tasks.push_back( std::pair<tarch::multicore::Task*, int>(task, taskNumber) );
}

