#include "FusableTasksQueue.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"


const std::string tarch::multicore::taskfusion::FusableTasksQueue::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::process-fused-tasks";


tarch::multicore::taskfusion::FusableTasksQueue::FusableTasksQueue(int type_):
  type(type_),
  numberOfPendingReadyTasksPerProcessTaskExecution(),
  probabilityThatTaskIsCompleted(256)
{}


void tarch::multicore::taskfusion::FusableTasksQueue::insertPendingTask( int taskNumber ) {
  tarch::multicore::Lock lock(pendingTasksSemaphore);
  pendingTasks.insert( taskNumber );
}


void tarch::multicore::taskfusion::FusableTasksQueue::insertReadyTask( Task* task, int taskNumber ) {
  tarch::multicore::Lock lock(readyTasksSemaphore);
  readyTasks[taskNumber] = task;

  assertion( pendingTasks.count(taskNumber)==0 );
}


void tarch::multicore::taskfusion::FusableTasksQueue::movePendingTaskIntoReadyTaskQueue( Task* task, int taskNumber ) {
  tarch::multicore::Lock readyTasksLock(readyTasksSemaphore);
  tarch::multicore::Lock pendingTasksLock(pendingTasksSemaphore);

  assertion( pendingTasks.count(taskNumber)==1 );

  pendingTasks.erase( taskNumber );
  readyTasks[taskNumber] = task;

  assertion( pendingTasks.count(taskNumber)==0 );
}


void tarch::multicore::taskfusion::FusableTasksQueue::drainReadyTasks() {
  tarch::multicore::Lock readyTaskLock(readyTasksSemaphore);

  for (auto& p: readyTasks) {
    p.second->run();
    delete p.second;
  }

  readyTasks.clear();
}


void tarch::multicore::taskfusion::FusableTasksQueue::processOneTask(
  int                taskNumber
) {
  tarch::multicore::Lock readyTaskLock(readyTasksSemaphore);

  if ( taskNumber!=AnyTask and readyTasks.count(taskNumber)>0 ) {
    readyTasks[taskNumber]->run();
    delete readyTasks[taskNumber];
    readyTasks.erase(taskNumber);
  }
  else if ( taskNumber==AnyTask and not readyTasks.empty() ) {
    auto [taskNumber, taskPointer] = *readyTasks.begin();
    taskPointer->run();
    delete taskPointer;
    readyTasks.erase(taskNumber);
  }
}


void tarch::multicore::taskfusion::FusableTasksQueue::processTasks() {
  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(type);

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
    readyTaskLock.free();

    tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTaskNumbers.size());
    processFusedTask(firstTask, extractedTaskPointers, instruction.device);
  }
}


bool tarch::multicore::taskfusion::FusableTasksQueue::hasTaskBeenProcessed(
  int                 taskNumber,
  bool                tryLock
) {
  tarch::multicore::Lock pendingLock(pendingTasksSemaphore, false);
  tarch::multicore::Lock readyLock(readyTasksSemaphore, false);

  if (tryLock) {
    if (
      not pendingLock.tryLock()
      or
      not readyLock.tryLock()
    ) {
      return false;
    }
  }
  else {
    pendingLock.lock();
    readyLock.lock();
  }

  return pendingTasks.count(taskNumber) == 0
     and readyTasks.count(taskNumber) == 0;
}
