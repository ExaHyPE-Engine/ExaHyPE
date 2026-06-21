// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "multicore.h"

#include "BooleanSemaphore.h"
#include "Core.h"
#include "Lock.h"
#include "Task.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/mpi/Rank.h"

void tarch::multicore::spawnTask(Task* task, const void* const inDependency, const void* const outDependency) {
  assertion(task != nullptr);

  if (inDependency == nullptr and outDependency == nullptr) {
    native::spawnTask(task);
  } else if (inDependency == nullptr and outDependency != nullptr) {
    native::spawnTask(task, outDependency);
  } else {
    native::spawnTask(task, inDependency, outDependency);
  }
}

void tarch::multicore::spawnTasks(const std::vector<Task*>& tasks) {
  if (not tasks.empty()) {
    native::spawnTasks(tasks);
  }
}

void tarch::multicore::waitForTask(const void* const inDependency) { native::waitForTask(inDependency); }

void tarch::multicore::waitForAllTasks() { native::waitForAllTasks(); }

#ifndef PEANO_WITH_MULTITHREADING

void tarch::multicore::native::spawnTask(Task* task, const void* const, const void* const) {
  while (task->run()) {
  };
  delete task;
}

void tarch::multicore::native::spawnTask(Task* task, const void* const) {
  while (task->run()) {
  };
  delete task;
}

void tarch::multicore::native::spawnTask(Task* task) {
  while (task->run()) {
  };
  delete task;
}

void tarch::multicore::native::spawnTasks(const std::vector<Task*>& tasks) {
  for (auto& p : tasks) {
    while (p->run()) {
    };
    delete p;
  }
}

void tarch::multicore::native::waitForTask(const void* const) {}

void tarch::multicore::native::waitForAllTasks() {}

#endif // PEANO_WITH_MULTITHREADING
