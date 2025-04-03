#include "ProcessReadyTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


const std::string tarch::multicore::taskfusion::ProcessReadyTask::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::process-fused-tasks";
tarch::logging::Log tarch::multicore::taskfusion::ProcessReadyTask::_log( "tarch::multicore::taskfusion::ProcessReadyTask" );


tarch::multicore::taskfusion::ProcessReadyTask::ProcessReadyTask( int taskNumber, ReadyFusableTasks* taskQueue ):
  Task(DontFuse,DefaultPriority),
  _taskNumber( taskNumber ),
  _taskQueue(taskQueue) {
  assertion( _taskQueue!=nullptr );
}


bool tarch::multicore::taskfusion::ProcessReadyTask::hasTaskBeenProcessed() const {
  tarch::multicore::Lock lock(_taskQueue->semaphore);
  return _taskQueue->completedTaskNumbers.find( _taskNumber ) != _taskQueue->completedTaskNumbers.end();
}


void tarch::multicore::taskfusion::ProcessReadyTask::removeProcessedTask() {
  tarch::multicore::Lock lock(_taskQueue->semaphore);
  ReadyFusableTasks::FinishedTaskNumbers::iterator p = _taskQueue->completedTaskNumbers.find( _taskNumber );
  if ( p != _taskQueue->completedTaskNumbers.end() ) {
    _taskQueue->completedTaskNumbers.erase(p);
  }
}


void tarch::multicore::taskfusion::ProcessReadyTask::run() {
  logDebug( "run(...)", "process all tasks of type " << _taskQueue->type << " (initial queue size=" << _taskQueue->tasks.size() << ")" );

  while ( not hasTaskBeenProcessed() ) {
    processTasks(_taskQueue, _taskNumber);
  }
  removeProcessedTask();
}


void tarch::multicore::taskfusion::ProcessReadyTask::processTasks(
  ReadyFusableTasks* taskQueue,
  int                taskNumber
) {
  tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(taskQueue->type);

  tarch::multicore::Lock lock(taskQueue->semaphore);
  const int queueSize = static_cast<int>(taskQueue->tasks.size());
  lock.free();

  if (instruction.minTasks <= queueSize) {
    assertion( instruction.maxTasks>=1 );
    fuse(taskQueue, instruction.maxTasks-1, instruction.device);
  }

  tryToProcessOneTask(taskQueue, taskNumber);
}


void tarch::multicore::taskfusion::ProcessReadyTask::tryToProcessOneTask(
  ReadyFusableTasks* taskQueue,
  int                taskNumber
) {
  Task* task = nullptr;

  tarch::multicore::Lock lock(taskQueue->semaphore);
  ReadyFusableTasks::Tasks::iterator p = taskQueue->tasks.begin();
  while (
    p!=taskQueue->tasks.end()
    and
    p->second != taskNumber
    and
    taskNumber>=0
  ) {
    p++;
  }

  if (p!=taskQueue->tasks.end()) {
    task = p->first;
    taskQueue->tasks.erase(p);
  }
  lock.free();

  if (task!=nullptr) {
    task->run();
    delete task;
    tarch::multicore::Lock lock(taskQueue->semaphore);
    taskQueue->completedTaskNumbers.insert( taskNumber );
  }
}



void tarch::multicore::taskfusion::ProcessReadyTask::fuse(
  ReadyFusableTasks* taskQueue,
  int maxTasks,
  int device
) {
  tarch::multicore::Lock lock(taskQueue->semaphore);

  if ( taskQueue->tasks.empty() ) return;

  auto [firstTask, firstTaskNumber] = *taskQueue->tasks.begin();
  taskQueue->tasks.pop_front();

  std::list< Task* > extractedTasks;
  std::list< int >   extractedTaskNumbers;

  while (
    maxTasks>0
    and
    not taskQueue->tasks.empty()
  ) {
    auto [nextTask, nextTaskNumber] = *taskQueue->tasks.begin();
    taskQueue->tasks.pop_front();
    extractedTasks.push_back(nextTask);
    extractedTaskNumbers.push_back(nextTaskNumber);
    maxTasks--;
  }
  logDebug( "run(...)", "run " << extractedTasks.size() << " tasks on top of first task" );

  lock.free();

  tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTasks.size());

  processFusedTask(firstTask, extractedTasks, device);

  lock.lock();
  taskQueue->completedTaskNumbers.insert( firstTaskNumber );
  for (auto& p: extractedTaskNumbers) {
    taskQueue->completedTaskNumbers.insert( p );
  }
}
