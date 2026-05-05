// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"

#if defined(WITH_OPENMP)

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Task.h"

void tarch::multicore::native::spawnTasks(const std::vector<Task*>& tasks) {
  assertion(not tasks.empty());
  logDebug("Spawn " << tasks.size() << " tasks");

#if not defined(TASKLOOP_NOT_SUPPORTED)
#pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(Task::DefaultPriority) untied
#endif
  for (std::size_t i = 0; i < tasks.size(); ++i) {
    auto* p = tasks[i];

#if defined(TASKLOOP_NOT_SUPPORTED)
#pragma omp task priority(Task::DefaultPriority) untied
#endif
    {
      while (p->run()) {
        Core::getInstance().yield();
      }
      delete p;
    }
  }

  waitForAllTasks();
}

void tarch::multicore::native::spawnTask(Task* task, const void* const inDependency, const void* const outDependency) {
#if defined(TASK_DEPENDENCIES_NOT_SUPPORTED)
#pragma omp task priority(task->getPriority()) untied
#else
  assertion(inDependency);
  assertion(outDependency);

  logDebug(
    "Fire task "
    << "with outgoing data dependency " << outDependency << " and "
    << "incoming data dependency " << inDependency
  );

#pragma omp task priority(task->getPriority()) depend(in : static_cast<const char*>(inDependency)[0]) \
  depend(out : static_cast<const char*>(outDependency)[0]) untied
#endif
  {
    while (task->run()) {
      Core::getInstance().yield();
    }
    delete task;
  }
}

void tarch::multicore::native::spawnTask(Task* task, const void* const outDependency) {
#if defined(TASK_DEPENDENCIES_NOT_SUPPORTED)
#pragma omp task priority(task->getPriority()) untied
#else
  assertion(outDependency);
  logDebug("Fire task with with outgoing data dependency " << outDependency);

#pragma omp task priority(task->getPriority()) depend(out : static_cast<const char*>(outDependency)[0]) untied
#endif
  {
    while (task->run()) {
      Core::getInstance().yield();
    }
    delete task;
  }
}

void tarch::multicore::native::spawnTask(Task* task) {
  logDebug("Fire independent task");

#pragma omp task priority(task->getPriority()) untied
  {
    while (task->run()) {
      Core::getInstance().yield();
    }
    delete task;
  }
}

void tarch::multicore::native::waitForTask(const void* const inDependency) {
#if defined(TASK_DEPENDENCIES_NOT_SUPPORTED)
#pragma omp taskwait
#else
  assertion(inDependency);
  logDebug("Wait for task with incoming data dependency " << inDependency << " to terminate");
#pragma omp taskwait depend(in : static_cast<const char*>(inDependency)[0])
#endif
}

void tarch::multicore::native::waitForAllTasks() {
#pragma omp taskwait
}

#endif
