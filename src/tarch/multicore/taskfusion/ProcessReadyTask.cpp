#include "ProcessReadyTask.h"

#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


const std::string tarch::multicore::taskfusion::ProcessReadyTask::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::process-fused-tasks";
tarch::logging::Log tarch::multicore::taskfusion::ProcessReadyTask::_log( "tarch::multicore::taskfusion::ProcessReadyTask" );


tarch::multicore::taskfusion::ProcessReadyTask::ProcessReadyTask( ReadyFusableTasks* taskQueue ):
  Task(DontFuse,DefaultPriority),
  _taskQueue(taskQueue) {
}


bool tarch::multicore::taskfusion::ProcessReadyTask::run() {
  tarch::multicore::Lock lock(_taskQueue->semaphore);
  const int initialQueueSize = _taskQueue->tasks.size();
  int processedTasks = 0;

  logDebug( "run(...)", "process all tasks of type " << _taskQueue->type << " (initial queue size=" << initialQueueSize << ")" );

  while (
    not _taskQueue->tasks.empty()
    and
    // exploit LIFO
    processedTasks < initialQueueSize
  ) {
    // can change from call to call
    tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(_taskQueue->type);

    Task* firstTask = *_taskQueue->tasks.begin();
    _taskQueue->tasks.pop_front();

    int maxTasks = std::min(static_cast<int>(_taskQueue->tasks.size()), instruction.maxTasks-1);
    ReadyFusableTasks::Tasks::iterator cutIterator = _taskQueue->tasks.begin();
    std::advance(cutIterator, maxTasks);
    ReadyFusableTasks::Tasks extractedTasks;
    extractedTasks.splice(extractedTasks.end(), _taskQueue->tasks, _taskQueue->tasks.begin(), cutIterator);

    logDebug( "run(...)", "max tasks on top of first task=" << maxTasks );

    lock.free();

    ::tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTasks.size());
    processedTasks += extractedTasks.size()+1;

    if ( instruction.minTasks>_taskQueue->tasks.size()+1 ) {
      logDebug( "run(...)", "here we go with " << extractedTasks.size() << " task(s) as we have to fuse at least " << instruction.minTasks << " task(s)" );
      for (auto& task: extractedTasks) {
        while (task->run()) {
          tarch::multicore::Core::getInstance().yield();
        }
        delete task;
      }
      logDebug( "run(...)", "now run first task manually" );
      while (firstTask->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete firstTask;
    }
    else {
      logDebug( "run(...)", "fuse them" );
      native::processFusedTask(firstTask, extractedTasks, instruction.device);
    }

    lock.lock();
  }

  return false;
}
