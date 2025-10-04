#include "FusableTasksQueue.h"
#include "ProcessOneReadyTask.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"



tarch::logging::Log  tarch::multicore::taskfusion::FusableTasksQueue::_log( "tarch::multicore::taskfusion::FusableTasksQueue" );


tarch::multicore::taskfusion::FusableTasksQueue::FusableTasksQueue(int type):
  _type(type),
  _numberOfReadyTasksPerProcessTaskExecution(tarch::timing::GlidingAverageMeasurement::DefaultWeight, 256),
  _probabilityThatTaskIsCompleted(tarch::timing::GlidingAverageMeasurement::DefaultWeight, 256) {}


void tarch::multicore::taskfusion::FusableTasksQueue::insertReadyTask( Task* task, int taskNumber ) {
  assertion1( _readyTasks.count(taskNumber)==0, taskNumber );

  tarch::multicore::Lock lock(_readyTasksSemaphore);
  _readyTasks[taskNumber] = task;
}


void tarch::multicore::taskfusion::FusableTasksQueue::drainReadyTasks() {
  int nextTaskNumber = AnyTask+1;

  while (nextTaskNumber!=AnyTask) {
    tarch::multicore::Lock readyTaskLock(_readyTasksSemaphore);
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


int tarch::multicore::taskfusion::FusableTasksQueue::getNumberOfReadyTasks() const {
  return _readyTasks.size();
}
