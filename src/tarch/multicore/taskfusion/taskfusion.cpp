// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "taskfusion.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"

#include <list>
#include <map>

namespace {
  /**
   * We map task types onto lists of tasks
   */
  std::map<int, std::list<tarch::multicore::Task*>> pendingTasksThatCanBeFused;
  tarch::multicore::BooleanSemaphore                pendingTasksSemaphore;

  tarch::logging::Log _log("tarch::multicore::taskfusion");

  const std::string SubmitFusedTaskStatisticsIdentifier("tarch::multicore::taskfusion::submit-fused-tasks");
} // namespace

void tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence(Task* task) {
  assertion(task->canFuse());
  assertion1(task->getTaskType() >= 0, task->getTaskType());

  logging::Statistics::getInstance().inc(SubmitFusedTaskStatisticsIdentifier, 1, false, true);

  Lock lock(pendingTasksSemaphore);

  if (pendingTasksThatCanBeFused.count(task->getTaskType()) == 0) {
    logDebug(
      "translateFusableTaskIntoTaskSequence(...)",
      "Insert new queue for task type " << task->getTaskType() << " into the sequence"
    );
    pendingTasksThatCanBeFused[task->getTaskType()] = std::list<Task*>();
  }

  pendingTasksThatCanBeFused[task->getTaskType()].push_back(task);
}

void tarch::multicore::taskfusion::processPendingTasksThatCanBeFused() {
  for (auto& [taskType, taskList] : pendingTasksThatCanBeFused) {
    if (!taskList.empty()) {
      // Get the first task and remove it from the list
      Task* firstTask = taskList.front();
      taskList.pop_front();

      // Fuse the first task with the remaining tasks in the list
      // The fuse also deletes every task in the list
      firstTask->fuse(taskList);

      // Clear the remaining tasks since they've been fused
      taskList.clear();

      delete firstTask;
    }
  }
}
