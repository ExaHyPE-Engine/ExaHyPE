#include "FusableTasksQueue.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"


const std::string tarch::multicore::taskfusion::FusableTasksQueue::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::process-fused-tasks";


tarch::multicore::taskfusion::FusableTasksQueue::FusableTasksQueue(int type):
  _type(type),
  numberOfPendingReadyTasksPerProcessTaskExecution(),
  probabilityThatTaskIsCompleted(256)
{}


void tarch::multicore::taskfusion::FusableTasksQueue::insertPendingTask( int taskNumber ) {
  {
    tarch::multicore::Lock lock(runningTasksSemaphore);
    runningTasks.erase(taskNumber);
  }
  {
    tarch::multicore::Lock lock(pendingTasksSemaphore);
    pendingTasks.insert( taskNumber );
  }
}


void tarch::multicore::taskfusion::FusableTasksQueue::insertReadyTask( Task* task, int taskNumber ) {
  {
    tarch::multicore::Lock lock(runningTasksSemaphore);
    runningTasks.erase(taskNumber);
  }
  {
    tarch::multicore::Lock lock(readyTasksSemaphore);
    readyTasks[taskNumber] = task;

    assertion( pendingTasks.count(taskNumber)==0 );
  }
}


void tarch::multicore::taskfusion::FusableTasksQueue::movePendingTaskIntoReadyTaskQueue( Task* task, int taskNumber ) {
  tarch::multicore::Lock readyTasksLock(readyTasksSemaphore);
  tarch::multicore::Lock pendingTasksLock(pendingTasksSemaphore);

  assertion( pendingTasks.count(taskNumber)==1 );

  pendingTasks.erase( taskNumber );
  readyTasks[taskNumber] = task;

  assertion( pendingTasks.count(taskNumber)==0 );
  assertion( runningTasks.count(taskNumber)==0 );
}


void tarch::multicore::taskfusion::FusableTasksQueue::drainReadyTasks() {
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
}


void tarch::multicore::taskfusion::FusableTasksQueue::processOneTask(
  int                taskNumber
) {
  tarch::multicore::Lock readyTaskLock(readyTasksSemaphore);

  if ( taskNumber==AnyTask and not readyTasks.empty() ) {
    taskNumber = readyTasks.begin()->first;
  }

  if ( readyTasks.count(taskNumber)>0 ) {
    tarch::multicore::Lock runningTaskLock(runningTasksSemaphore);
    runningTasks.insert( taskNumber );
    runningTaskLock.free();

    Task* task = readyTasks[taskNumber];
    readyTasks.erase(taskNumber);
    readyTaskLock.free();

    task->run();
    delete task;

    runningTaskLock.lock();
    runningTasks.erase( taskNumber );
  }
}


void tarch::multicore::taskfusion::FusableTasksQueue::processTasks() {
  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(getType());

  assertion( instruction.minTasks>=1 );
  assertion( instruction.maxTasks>=1 );

  tarch::multicore::Lock readyTaskLock(readyTasksSemaphore);
  const int queueSize = static_cast<int>(readyTasks.size());

  if (instruction.minTasks <= queueSize) {
    assertion( instruction.maxTasks>=1 );

    // extract first task
    auto [firstTaskNumber, firstTask] = *readyTasks.begin();
    readyTasks.erase( firstTaskNumber );

    // extract sequence
    int tasksToExtract = instruction.maxTasks-1;
    Tasks::iterator iterator = readyTasks.begin();

    std::list< Task* > extractedTaskPointers;
    std::list< int >   extractedTaskNumbers;
    while (tasksToExtract>0 and iterator != readyTasks.end()) {
      tasksToExtract--;
      extractedTaskNumbers.push_back( iterator->first );
      extractedTaskPointers.push_back( iterator->second );
      iterator = readyTasks.erase( iterator );
    }

    tarch::multicore::Lock runningTaskLock(runningTasksSemaphore);
    runningTasks.insert( firstTaskNumber );
    runningTasks.insert( extractedTaskNumbers.begin(), extractedTaskNumbers.end() );
    runningTaskLock.free();

    readyTaskLock.free();

    tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTaskNumbers.size());
    processFusedTask(firstTask, extractedTaskPointers, instruction.device);

    runningTaskLock.lock();
    runningTasks.erase( firstTaskNumber );
    for (auto& taskNumber: extractedTaskNumbers) {
      runningTasks.erase( taskNumber );
    }
    runningTaskLock.free();
  }
}


bool tarch::multicore::taskfusion::FusableTasksQueue::isTaskPending(
  int                 taskNumber,
  bool                tryLock
) {
  tarch::multicore::Lock pendingLock(pendingTasksSemaphore, false);
  tarch::multicore::Lock readyLock(readyTasksSemaphore, false);
  tarch::multicore::Lock runningLock(runningTasksSemaphore, false);

  if (tryLock) {
    if (
      not pendingLock.tryLock()
      or
      not readyLock.tryLock()
      or
      not runningLock.tryLock()
    ) {
      return true;
    }
  }
  else {
    pendingLock.lock();
    readyLock.lock();
    runningLock.lock();
  }

  return pendingTasks.count(taskNumber) > 0
      or readyTasks.count(taskNumber) > 0
      or runningTasks.count(taskNumber) > 0;
}


int tarch::multicore::taskfusion::FusableTasksQueue::getType() const {
  return _type;
}
