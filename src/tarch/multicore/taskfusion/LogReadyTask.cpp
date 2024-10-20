#include "LogReadyTask.h"

#include "tarch/multicore/Lock.h"

#include "tarch/logging/Statistics.h"


const std::string tarch::multicore::taskfusion::LogReadyTask::SubmitFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::submit-fused-tasks";
tarch::logging::Log tarch::multicore::taskfusion::LogReadyTask::_log( "tarch::multicore::taskfusion::LogReadyTask" );


tarch::multicore::taskfusion::LogReadyTask::LogReadyTask( Task* task, ReadyFusableTasks* taskQueue ):
  Task(DontFuse,DefaultPriority),
  _task(task),
  _taskQueue(taskQueue) {
}


bool tarch::multicore::taskfusion::LogReadyTask::run() {
  ::tarch::logging::Statistics::getInstance().inc(SubmitFusedTasksStatisticsIdentifier, 1, false, true);

  logDebug( "run(...)", "task of type " << _task->getTaskType() << " is now ready to run. Put into ready queue" );

  tarch::multicore::Lock lock(_taskQueue->semaphore);
  _taskQueue->tasks.push_back( _task );
}
