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

/**
 * Spawns a single task in a non-blocking fashion
 *
 * Ownership goes over to Peano's multicore namespace, i.e. the caller does not
 * have to delete the pointer.
 *
 * ## Handling tasks without outgoing dependencies
 *
 * An outDependency of nullptr means that nobody is directly waiting for this
 * task, so no dependencies are added to the task graph afterwards.
 *
 * spawnTask() ***never*** commits a task to the global task queue and
 * therefore is inherently thread-safe.
 *
 * @see tarch::multicore and the section "Tasks with dependencies" therein
 *   for further documentation.
 *
 * @param task Pointer to a task. The responsibility for this task is handed
 *    over to the tasking system, i.e. you are not allowed to delete it.
 * @param inDependency Incoming data dependency that has to finish before the
 *    present task is allowed to run. You can pass nullptr to indicate no
 *    dependency.
 * @param outDependency Outgoing data dependency of the task. You can pass
 *    nullptr to indicate no dependency.
 */
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
