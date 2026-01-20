#include "ProcessSetOfReadyTasks.h"
#include "FusableTasksQueue.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log tarch::multicore::taskfusion::ProcessSetOfReadyTasks::_log( "tarch::multicore::taskfusion::ProcessSetOfReadyTasks" );
const std::string tarch::multicore::taskfusion::ProcessSetOfReadyTasks::ProcessFusedTasksStatisticsIdentifier = "tarch::multicore::taskfusion::ProcessSetOfReadyTasks::ProcessFusedTasksStatisticsIdentifier";


tarch::multicore::taskfusion::ProcessSetOfReadyTasks::ProcessSetOfReadyTasks(
  TaskNumber          taskNumber,
  FusableTasksQueue&  taskQueue
):
  Task(DontFuse,DefaultPriority),
  _taskQueue( taskQueue ),
  _taskNumber( taskNumber ) {
  assertion( _taskNumber!= tarch::multicore::taskfusion::FusableTasksQueue::AnyTask );
}


void tarch::multicore::taskfusion::ProcessSetOfReadyTasks::run() {
  if ( _taskQueue.isTaskReadyOrRunning(_taskNumber, FusableTasksQueue::LockQueueAndReturnReliableResult) ) {
    tarch::multicore::orchestration::Strategy::FuseInstruction instruction = tarch::multicore::getOrchestration().fuse(_taskQueue.getType());
    assertion( instruction.minTasks>=1 );
    assertion( instruction.maxTasks>=1 );

    const bool triggerFusion = instruction.minTasks<=_taskQueue.getNumberOfReadyTasks();

    _taskQueue.updateTaskFusionProbabilities(triggerFusion);

    if ( triggerFusion ) {
      auto [firstTaskNumber, firstTask] = _taskQueue.extractFirstReadyTask(_taskNumber);

      if (firstTask!=nullptr) {
        int tasksToExtract = std::min( instruction.maxTasks-1, static_cast<int>(_taskQueue.getNumberOfReadyTasks()) );

        std::vector< Task* > extractedTaskPointers;
        std::vector< int >   extractedTaskNumbers;

        extractedTaskPointers.reserve(tasksToExtract);
        extractedTaskNumbers.reserve(tasksToExtract);

        while (tasksToExtract>0) {
          auto [taskNumber, task] = _taskQueue.extractFirstReadyTask(_taskNumber);

          if (task!=nullptr) {
            extractedTaskNumbers.push_back( taskNumber );
            extractedTaskPointers.push_back( task );
            tasksToExtract--;
          }
          else {
            tasksToExtract = 0;
          }
        }

        tarch::logging::Statistics::getInstance().log(ProcessFusedTasksStatisticsIdentifier, extractedTaskNumbers.size());
        processFusedTask(firstTask, extractedTaskPointers, instruction.device);

        _taskQueue.runningTaskFinished( firstTaskNumber );
        for (auto& taskNumber: extractedTaskNumbers) {
          _taskQueue.runningTaskFinished( taskNumber );
        }
      }

      logDebug( "run(...)", "fused task " << _taskNumber << " finished" );
    }
  }
}


void tarch::multicore::taskfusion::ProcessSetOfReadyTasks::processFusedTask(Task* myTask, const std::vector<tarch::multicore::Task*>& tasksOfSameType, int device) {
  myTask->fuse(tasksOfSameType, device);
  for (auto& p: tasksOfSameType) {
    delete p;
  }
  delete myTask;
}
