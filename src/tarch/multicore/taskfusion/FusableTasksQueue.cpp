#include "FusableTasksQueue.h"
#include "ProcessOneReadyTask.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"



tarch::logging::Log  tarch::multicore::taskfusion::FusableTasksQueue::_log( "tarch::multicore::taskfusion::FusableTasksQueue" );
const std::string tarch::multicore::taskfusion::FusableTasksQueue::SizeOfFusableTaskQueueStatisticsIdentifier("tarch::multicore::taskfusion::FusableTasksQueue::SizeOfFusableTaskQueueStatisticsIdentifier");


tarch::multicore::taskfusion::FusableTasksQueue::FusableTasksQueue(int type):
  _type(type),
  _numberOfReadyTasksPerProcessTaskExecution(tarch::timing::GlidingAverageMeasurement::DefaultWeight, 256),
  _probabilityThatTaskIsCompleted(tarch::timing::GlidingAverageMeasurement::DefaultWeight, 256) {}


void tarch::multicore::taskfusion::FusableTasksQueue::insertReadyTask( Task* task, int taskNumber ) {
  tarch::multicore::Lock lock(_readyTasksSemaphore);
  assertion1( _readyTasks.count(taskNumber)==0, taskNumber );
  _readyTasks[taskNumber] = task;
}


std::pair< tarch::multicore::TaskNumber, tarch::multicore::Task* > tarch::multicore::taskfusion::FusableTasksQueue::extractFirstReadyTask( TaskNumber handlingTask ) {
  tarch::multicore::Lock readyTaskLock(_readyTasksSemaphore);
  ::tarch::logging::Statistics::getInstance().log(SizeOfFusableTaskQueueStatisticsIdentifier + "(" + std::to_string(_type) + ")", _readyTasks.size());
  if ( _readyTasks.empty() ) {
    return {-1, nullptr};
  }
  else {
    tarch::multicore::Lock runningTaskLock(_runningTasksSemaphore);
    auto [firstTaskNumber, firstTask] = *_readyTasks.begin();
    _readyTasks.erase( _readyTasks.begin() );
    _runningTasks[ firstTaskNumber ] = handlingTask;
    return {firstTaskNumber, firstTask};
  }
}


tarch::multicore::Task*  tarch::multicore::taskfusion::FusableTasksQueue::extractReadyTask( TaskNumber taskNumber, TaskNumber handlingTask ) {
  tarch::multicore::Lock readyTaskLock(_readyTasksSemaphore);
  if ( _readyTasks.count(taskNumber)>0 ) {
    logDebug( "extractReadyTask()", "task " << taskNumber << " is ready, i.e. not running yet. Execute it manually" );
    tarch::multicore::Lock runningTaskLock(_runningTasksSemaphore);

    _runningTasks[ taskNumber ] = handlingTask;
    Task* task = _readyTasks[taskNumber];
    _readyTasks.erase(taskNumber);

    ::tarch::logging::Statistics::getInstance().log(SizeOfFusableTaskQueueStatisticsIdentifier + "(" + std::to_string(_type) + ")", _readyTasks.size());

    return task;
  }
  else {
    return nullptr;
  }
}


tarch::multicore::TaskNumber tarch::multicore::taskfusion::FusableTasksQueue::getTaskHandlingRunningTask( TaskNumber taskNumber ) {
  tarch::multicore::Lock runningTaskLock(_readyTasksSemaphore);
  if (_runningTasks.count(taskNumber)) {
    return _runningTasks[taskNumber];
  }
  else {
    return NoOutDependencies;
  }
}



void tarch::multicore::taskfusion::FusableTasksQueue::runningTaskFinished( TaskNumber taskNumber ) {
  tarch::multicore::Lock runningTaskLock(_runningTasksSemaphore);
  assertion( _runningTasks.count( taskNumber )==1 );
  _runningTasks.erase( taskNumber );
}


void tarch::multicore::taskfusion::FusableTasksQueue::updateTaskFusionProbabilities( bool triggerFusion ) {
  tarch::multicore::Lock lock(_statisticsSemaphore);

  if (triggerFusion) {
    tarch::multicore::Lock readyQueueLock(_readyTasksSemaphore);
    const int readyQueueSize = _readyTasks.size();
    readyQueueLock.free();

    _numberOfReadyTasksPerProcessTaskExecution.setValue( readyQueueSize );
    _probabilityThatTaskIsCompleted.setValue(0.0);

    if (_probabilityThatTaskIsCompleted.hasFullSetOfMeasurements()) {
      tarch::multicore::getOrchestration().updateFuseStatistics(
        getType(),
        readyQueueSize,
        _numberOfReadyTasksPerProcessTaskExecution.getValue(),
        _probabilityThatTaskIsCompleted.getValue()
      );
    }
  }
  else {
    _probabilityThatTaskIsCompleted.setValue(1.0);
  }
}


tarch::multicore::TaskNumber tarch::multicore::taskfusion::FusableTasksQueue::getFirstReadyTaskNumber() {
  tarch::multicore::Lock readyTaskLock(_readyTasksSemaphore);
  if ( _readyTasks.empty() ) {
    return tarch::multicore::NoOutDependencies;
  }
  else {
    return _readyTasks.begin()->first;
  }
}


void tarch::multicore::taskfusion::FusableTasksQueue::drainReadyTasks() {
  int nextTaskNumber = AnyTask+1;

  while (nextTaskNumber!=AnyTask) {
    tarch::multicore::Lock readyTaskLock(_readyTasksSemaphore);
    ::tarch::logging::Statistics::getInstance().log(SizeOfFusableTaskQueueStatisticsIdentifier + "(" + std::to_string(_type) + ")", _readyTasks.size());
    nextTaskNumber = AnyTask;
    if (not _readyTasks.empty()) {
      nextTaskNumber = _readyTasks.begin()->first;
    }
    readyTaskLock.free();

    if (nextTaskNumber!=AnyTask) {
      tarch::multicore::Task* newProcessTask = new tarch::multicore::taskfusion::ProcessOneReadyTask(nextTaskNumber, *this);
      tarch::multicore::spawnTask(
        newProcessTask,
        {nextTaskNumber},
        nextTaskNumber
      );
      tarch::multicore::waitForTask(nextTaskNumber, TaskWaitType::OnlyNativeTasks);
    }
  }
}


bool tarch::multicore::taskfusion::FusableTasksQueue::isTaskReadyOrRunning(
  int                 taskNumber,
  bool                tryLock
) {
  tarch::multicore::Lock readyLock(_readyTasksSemaphore, false);
  tarch::multicore::Lock runningLock(_runningTasksSemaphore, false);

  if (tryLock) {
    if (
      not readyLock.tryLock()
      or
      not runningLock.tryLock()
    ) {
      return true;
    }
  }
  else {
    readyLock.lock();
    runningLock.lock();
  }

  return _readyTasks.count(taskNumber) > 0
      or _runningTasks.count(taskNumber) > 0;
}


bool tarch::multicore::taskfusion::FusableTasksQueue::isTaskReady(
  int                 taskNumber,
  bool                tryLock
) {
  tarch::multicore::Lock readyLock(_readyTasksSemaphore, false);

  if (tryLock) {
    if (
      not readyLock.tryLock()
    ) {
      return true;
    }
  }
  else {
    readyLock.lock();
  }

  return _readyTasks.count(taskNumber) > 0;
}


int tarch::multicore::taskfusion::FusableTasksQueue::getType() const {
  return _type;
}


int tarch::multicore::taskfusion::FusableTasksQueue::getNumberOfReadyTasks() {
  int result;

  tarch::multicore::Lock readyLock(_readyTasksSemaphore);
  result = _readyTasks.size();

  return result;
}
