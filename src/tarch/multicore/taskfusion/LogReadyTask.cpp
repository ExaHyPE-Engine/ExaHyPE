#include "LogReadyTask.h"
#include "FusableTasksQueue.h"

#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/Assertions.h"


const std::string tarch::multicore::taskfusion::LogReadyTask::SubmitFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::LogReadyTask::SubmitFusedTasksStatisticsIdentifier";
tarch::logging::Log tarch::multicore::taskfusion::LogReadyTask::_log( "tarch::multicore::taskfusion::LogReadyTask" );
tarch::multicore::TaskEnumerator tarch::multicore::taskfusion::LogReadyTask::_enumerator;


tarch::multicore::taskfusion::LogReadyTask::LogReadyTask( Task* task, int taskNumber, FusableTasksQueue& taskQueue ):
  Task(DontFuse,DefaultPriority),
  _task(task),
  _taskNumber(taskNumber==AdministerYourOwnTemporaryTaskLocalNumber ? _enumerator.getNumber() : taskNumber),
  _taskQueue(taskQueue),
  _administerYourOwnTemporaryTaskLocalNumber(taskNumber==AdministerYourOwnTemporaryTaskLocalNumber) {
  assertion1( taskNumber==AdministerYourOwnTemporaryTaskLocalNumber or taskNumber>=0, taskNumber );
}


void tarch::multicore::taskfusion::LogReadyTask::run() {
  ::tarch::logging::Statistics::getInstance().inc(SubmitFusedTasksStatisticsIdentifier, 1, false, true);

  logDebug( "run(...)", "task of type " << _task->getTaskType() << " is now ready to run. Put into ready queue" );

  _taskQueue.insertReadyTask(_task, _taskNumber );

  if (_administerYourOwnTemporaryTaskLocalNumber) {
    _enumerator.releaseNumber( _taskNumber );
  }
}


tarch::multicore::TaskNumber tarch::multicore::taskfusion::LogReadyTask::getNumber() const {
  return _taskNumber;
}
