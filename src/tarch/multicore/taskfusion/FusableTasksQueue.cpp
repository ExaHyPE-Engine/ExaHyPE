#include "FusableTasksQueue.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"



tarch::logging::Log  tarch::multicore::taskfusion::FusableTasksQueue::_log( "tarch::multicore::taskfusion::FusableTasksQueue" );


tarch::multicore::taskfusion::FusableTasksQueue::FusableTasksQueue(int type):
  _type(type),
  _numberOfPendingReadyTasksPerProcessTaskExecution(),
  _probabilityThatTaskIsCompleted(256)
{}


void tarch::multicore::taskfusion::FusableTasksQueue::insertPendingTask( int taskNumber ) {
  assertion1( _pendingTasks.count(taskNumber)==0, taskNumber );
  assertion1( _readyTasks.count(taskNumber)==0, taskNumber );

  tarch::multicore::Lock lock(_pendingTasksSemaphore);
  _pendingTasks.insert( taskNumber );
}


void tarch::multicore::taskfusion::FusableTasksQueue::insertReadyTask( Task* task, int taskNumber ) {
  assertion1( _pendingTasks.count(taskNumber)==0, taskNumber );
  assertion1( _readyTasks.count(taskNumber)==0, taskNumber );

  tarch::multicore::Lock lock(_readyTasksSemaphore);
  _readyTasks[taskNumber] = task;
}


void tarch::multicore::taskfusion::FusableTasksQueue::movePendingTaskIntoReadyTaskQueue( Task* task, int taskNumber ) {
  tarch::multicore::Lock readyTasksLock(_readyTasksSemaphore);
  tarch::multicore::Lock pendingTasksLock(_pendingTasksSemaphore);

  assertion( _pendingTasks.count(taskNumber)==1 );

  _pendingTasks.erase( taskNumber );
  _readyTasks[taskNumber] = task;

  assertion( _pendingTasks.count(taskNumber)==0 );
}


void tarch::multicore::taskfusion::FusableTasksQueue::drainReadyTasks() {
  // @todo Pleaes complete with tasks
/*
  tarch::multicore::Lock readyTaskLock(readyTasksSemaphore);

  for (auto& p: readyTasks) {
    int taskNumber = p.first;
    tarch::multicore::Lock runningTaskLock(runningTasksSemaphore);
    runningTasks.insert( taskNumber );
    runningTaskLock.free();

    p.second->run();
    delete p.second;

    runningTaskLock.lock();
    runningTasks.erase( taskNumber );
  }

  readyTasks.clear();
*/
}


bool tarch::multicore::taskfusion::FusableTasksQueue::isTaskPending(
  int                 taskNumber,
  bool                tryLock
) {
  tarch::multicore::Lock pendingLock(_pendingTasksSemaphore, false);

  if (tryLock) {
    if (
      not pendingLock.tryLock()
    ) {
      return true;
    }
  }
  else {
    pendingLock.lock();
  }

  return _pendingTasks.count(taskNumber) > 0;
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


int tarch::multicore::taskfusion::FusableTasksQueue::getNumberOfReadyTasks() const {
  return _readyTasks.size();
}
