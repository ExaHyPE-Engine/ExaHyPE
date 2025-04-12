#include "ProcessReadyTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


const std::string tarch::multicore::taskfusion::ProcessReadyTask::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::process-fused-tasks";
tarch::logging::Log tarch::multicore::taskfusion::ProcessReadyTask::_log( "tarch::multicore::taskfusion::ProcessReadyTask" );


tarch::multicore::taskfusion::ProcessReadyTask::ProcessReadyTask( int taskNumber, ReadyFusableTasks* _taskQueue ):
  Task(DontFuse,DefaultPriority),
  _taskNumber( taskNumber ),
  _taskQueue(_taskQueue) {
  assertion( _taskQueue!=nullptr );
}


bool tarch::multicore::taskfusion::ProcessReadyTask::hasTaskBeenProcessed() const {
  tarch::multicore::Lock lock(_taskQueue->semaphore);
  return _taskQueue->completedTaskNumbers.find( _taskNumber ) != _taskQueue->completedTaskNumbers.end();
}


void tarch::multicore::taskfusion::ProcessReadyTask::run() {
  logDebug( "run(...)", "process all tasks of type " << _taskQueue->type << " (initial queue size=" << _taskQueue->tasks.size() << ")" );

  tarch::multicore::Lock lock(_taskQueue->semaphore);
  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(_taskQueue->type);
  const bool hasTaskBeenProcessedWhenFunctionInvoked = hasTaskBeenProcessed();

  if (
    not hasTaskBeenProcessedWhenFunctionInvoked
  ) {
    _taskQueue->numberOfPendingReadyTasksPerProcessTaskExecution.setValue(_taskQueue->tasks.size());
    _taskQueue->probabilityThatTaskIsCompleted.setValue(1.0);
  }
  else {
    _taskQueue->probabilityThatTaskIsCompleted.setValue(0.0);
  }

  if (_taskQueue->numberOfPendingReadyTasksPerProcessTaskExecution.hasFullSetOfMeasurements()) {
    tarch::multicore::getOrchestration().updateFuseStatistics(
      _taskQueue->type,
      _taskQueue->tasks.size(),
      _taskQueue->numberOfPendingReadyTasksPerProcessTaskExecution.getValue(),
      _taskQueue->probabilityThatTaskIsCompleted.getValue()
    );
  }
  lock.free();

  if (
    not hasTaskBeenProcessedWhenFunctionInvoked
    and
    instruction.timeoutMicroSeconds>std::chrono::microseconds(0)
    and
    instruction.minTasks>=_taskQueue->tasks.size()
  ) {
    logDebug( "run(...)", "sleep for " << instruction.timeoutMicroSeconds << " milliseconds" );
    auto deadline = std::chrono::system_clock::now() + instruction.timeoutMicroSeconds;
    while (std::chrono::system_clock::now() < deadline) {
      std::this_thread::sleep_for( std::chrono::microseconds(1) );
    }
  }

  if (
    not hasTaskBeenProcessedWhenFunctionInvoked
    and
    instruction.minTasks<=_taskQueue->tasks.size()
  ) {
    logInfo( "run(...)", "fuse as we have " << _taskQueue->tasks.size() << " tasks" );
    processTasks();
  }

  if (
    not hasTaskBeenProcessedWhenFunctionInvoked
  ) {
    while ( not hasTaskBeenProcessed() ) {
      tryToProcessOneTask(_taskQueue, _taskNumber);
    }
  }
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



void tarch::multicore::taskfusion::ProcessReadyTask::processTasks() {
  tarch::multicore::Lock lock(_taskQueue->semaphore);
  const int queueSize = static_cast<int>(_taskQueue->tasks.size());

  const tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(_taskQueue->type);

  if (instruction.minTasks <= queueSize and not _taskQueue->tasks.empty()) {
    assertion( instruction.maxTasks>=1 );

    auto [firstTask, firstTaskNumber] = *_taskQueue->tasks.begin();
    _taskQueue->tasks.pop_front();

    std::list< Task* > extractedTasks;
    std::list< int >   extractedTaskNumbers;
    int                maxTasks = instruction.maxTasks;

    maxTasks--;
    while (
      maxTasks>0
      and
      not _taskQueue->tasks.empty()
    ) {
      auto [nextTask, nextTaskNumber] = *_taskQueue->tasks.begin();
      _taskQueue->tasks.pop_front();
      extractedTasks.push_back(nextTask);
      extractedTaskNumbers.push_back(nextTaskNumber);
      maxTasks--;
    }
    logDebug( "run(...)", "run " << extractedTasks.size() << " tasks on top of first task" );

    lock.free();

    tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTasks.size());
    processFusedTask(firstTask, extractedTasks, instruction.device);

    lock.lock();
    _taskQueue->completedTaskNumbers.insert( firstTaskNumber );
    for (auto& p: extractedTaskNumbers) {
      _taskQueue->completedTaskNumbers.insert( p );
    }
  }
}
