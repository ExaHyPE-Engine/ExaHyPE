// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveBookkeeping.h"

tarch::logging::Log exahype2::EnclaveBookkeeping::_log("exahype2::EnclaveBookkeeping");

exahype2::EnclaveBookkeeping::~EnclaveBookkeeping() {
  // clear();
}

void exahype2::EnclaveBookkeeping::bookmarkTask(tarch::multicore::Task* task) {
  assertion(task != nullptr);
  assertion1(task->getTaskType() >= 0, task->getTaskType());

  tarch::multicore::Lock lock(pendingTasksSemaphore);

  if (pendingTasks.count(task->getTaskType()) == 0) {
    logDebug("bookmarkTask()", "Construct new enclave task queue for task type " << task->getTaskType());
    pendingTasks[task->getTaskType()] = std::list<tarch::multicore::Task*>();
  }

  pendingTasks[task->getTaskType()].push_back(task);
}

void exahype2::EnclaveBookkeeping::clear() {
  for (auto& [taskType, taskList] : pendingTasks) {
    for (tarch::multicore::Task* task : taskList) {
      delete task;
    }
    taskList.clear();
  }
  pendingTasks.clear();
}

void exahype2::EnclaveBookkeeping::fusePendingTasks() {
  for (auto& [taskType, taskList] : pendingTasks) {
    if (!taskList.empty()) {
      // Get the first task and remove it from the list
      tarch::multicore::Task* firstTask = taskList.front();
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

std::map<int, std::size_t> exahype2::EnclaveBookkeeping::getNumberOfTasksPerType() const {
  std::map<int, std::size_t> result;
  for (const auto& [taskType, taskList] : pendingTasks) {
    result[taskType] = taskList.size();
  }
  return result;
}

std::size_t exahype2::EnclaveBookkeeping::getTotalNumberOfTasks() const {
  std::size_t count = 0;
  for (const auto& [_, taskList] : pendingTasks) {
    count += taskList.size();
  }
  return count;
}
