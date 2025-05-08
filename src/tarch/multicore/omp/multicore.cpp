// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"

#if defined(SharedOMP)

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Task.h"

namespace {
  static tarch::logging::Log _log("tarch::multicore::native(omp)");
} // namespace

void tarch::multicore::native::spawnTasks(const std::vector<Task*>& tasks) {
  assertion(not tasks.empty());
  logDebug("spawnTasks()", "Spawn " << tasks.size() << " tasks as taskloop");

#pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(Task::DefaultPriority) untied
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
    {
      while (tasks[i]->run()) {
        Core::getInstance().yield();
      }
      delete tasks[i];
    }
  }

  waitForAllTasks();
}

void tarch::multicore::native::spawnTask(Task* task, const void* const inDependency, const void* const outDependency) {
  assertion(inDependency);
  assertion(outDependency);

  logDebug(
    "spawnTask()",
    "Fire task "
      << "with outgoing data dependency " << outDependency << " and "
      << "incoming data dependency " << inDependency
  );

#pragma omp task priority(task->getPriority()) \
  depend(in : static_cast<const char*>(inDependency)[0]) \
  depend(out : static_cast<const char*>(outDependency)[0]) \
  untied
  {
    while (task->run()) {
      Core::getInstance().yield();
    }
    delete task;
  }
}

void tarch::multicore::native::spawnTask(Task* task, const void* const outDependency) {
  assertion(outDependency);
  logDebug("spawnTask()", "Fire task with with outgoing data dependency " << outDependency);

#pragma omp task priority(task->getPriority()) \
  depend(out : static_cast<const char*>(outDependency)[0]) \
  untied
  {
    while (task->run()) {
      Core::getInstance().yield();
    }
    delete task;
  }
}

void tarch::multicore::native::spawnTask(Task* task) {
  logDebug("spawnTask()", "Fire independent task");

#pragma omp task priority(task->getPriority()) untied
  {
    while (task->run()) {
      Core::getInstance().yield();
    }
    delete task;
  }
}

void tarch::multicore::native::waitForTask(const void* const inDependency) {
  assertion(inDependency);
  logDebug("waitForTask()", "Wait for task with incoming data dependency " << inDependency << " to terminate");
#pragma omp taskwait depend(in : static_cast<const char*>(inDependency)[0])
}

void tarch::multicore::native::waitForAllTasks() {
#pragma omp taskwait
}

#endif
