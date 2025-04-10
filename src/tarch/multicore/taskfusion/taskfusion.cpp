// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "taskfusion.h"

#include "tarch/Assertions.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <map>
#include <list>

namespace {
  /**
   * We map task types onto lists of tasks
   */
  std::map<int, std::list<tarch::multicore::Task*>> pendingTasksThatCanBeFused;
  tarch::multicore::BooleanSemaphore                pendingTasksSemaphore;
  static constexpr int FuseThreshold = -1;

  tarch::logging::Log _log("tarch::multicore::taskfusion");

  const std::string SubmitFusedTaskStatisticsIdentifier("tarch::multicore::taskfusion::submit-fused-tasks");
}

void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence(Task* task) {
  logging::Statistics::getInstance().inc(SubmitFusedTaskStatisticsIdentifier, 1, false, true);

  assertion1(task->getTaskType() >= 0, task->getTaskType());

  Lock lock(pendingTasksSemaphore);

  if (pendingTasksThatCanBeFused.count(task->getTaskType()) == 0) {
    logDebug("translateFusableTaskIntoTaskSequence(...)", "insert new queue for task type " << task->getTaskType() << " into the sequence");
    pendingTasksThatCanBeFused[task->getTaskType()] = std::list<Task*>();
  }

  // TODO: Move orchestration strategies into this namespace here
  #ifdef UseTargetDART
  int device = TARGETDART_ANY; // TODO: Use targetDART orchestration strategy
  #else
  int device = 0;
  #endif
  bool taskHasBeenFused = false;
  if constexpr (FuseThreshold > 0) {
    auto& taskList = pendingTasksThatCanBeFused[task->getTaskType()];
    if (taskList.size() >= FuseThreshold) { // TODO: Use proper orchestration strategy here
      task->fuse(taskList, device);
      taskList.clear();
      delete task;
      taskHasBeenFused = true;
    }
  }

  if (not taskHasBeenFused) pendingTasksThatCanBeFused[task->getTaskType()].push_back(task);
}

void tarch::multicore::taskfusion::processPendingTasksThatCanBeFused() {
  #ifdef UseTargetDART
  int device = TARGETDART_ANY; // TODO: Use targetDART orchestration strategy
  #else
  int device = 0;
  #endif

  logDebug("processPendingTasksThatCanBeFused(int)", "process pending tasks that can be fused on device " << device);

  for (auto& [taskType, taskList] : pendingTasksThatCanBeFused) {
    if (!taskList.empty()) {
      // Get the first task and remove it from the list
      Task* firstTask = taskList.front();
      taskList.pop_front();

      // Fuse the first task with the remaining tasks in the list
      // The fuse also deletes every task in the list
      firstTask->fuse(taskList, device);

      // Clear the remaining tasks since they've been fused
      taskList.clear(); // TODO: This should only be needed after the grid changes due to mesh refinement

      delete firstTask;
    }
  }
}
