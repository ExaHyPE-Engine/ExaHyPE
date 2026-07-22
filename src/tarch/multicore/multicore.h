// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch {
  /**
   @namespace tarch::multicore

   This page describes Peano 4's multithreading namespace. A more high-level
   overview is provided through @ref page_tarch_multicore.

   ## Writing your own code with multithreading features

   If you want to distinguish in your code between multicore and no-multicore variants,
   please use

  ~~~~~~~~~~~~~~~~~~~~~~~~
  #include "tarch/multicore/multicore.h"
  ~~~~~~~~~~~~~~~~~~~~~~~~

  and

  ~~~~~~~~~~~~~~~~~~~~~~~~
  #if defined(PEANO_WITH_MULTITHREADING)
  ~~~~~~~~~~~~~~~~~~~~~~~~

   With the symbol PEANO_WITH_MULTITHREADING, you make your code independent of
   OpenMP, TBB or C++ threading.

   ## Multicore architecture

   The multithreading environment is realised through a small set of classes. User
   codes work with these classes. Each type/function has an implementation within
   src/multicore. This implementation is a dummy that ensures that all code works
   properly without any multithreading support. Subdirectories hold alternative
   implementations (backends) which are enabled once the user selects a certain multithreading
   implementation variant, i.e. depending on the ifdefs set, one of the
   subdirectories is used. Some implementations introduce further headers, but user
   code is never supposed to work against functions or classes held within
   subdirectories.

    @image html multicore_architecture.png

   ## Statistics

   If the Peano statistics are enabled, the tasking back-end will sample several
   quantities such as "tarch::multicore::bsp-concurrency-level" or
   "tarch::multicore::spawned-tasks".
   Depending on the chosen back-end, you might get additional counters on top.
   Please start to look into tarch/multicore/Tasks.cpp for an overview of the
   most generic counters.

   ## Tasking model in Peano

   Peano models all of its interna as tasks. Each Peano 4 task is a subclass of
   tarch::multicore::Task. However, these classes might not be mapped 1:1 onto
   native tasks. In line with other APIs such as OneTBB, we distinguish different
   task types or task graph types, respectively:

   - ***Tasks***. The most generic type of tasks is submitted via spawnTask().
     Each task can be given an incoming and an outgoing dependency.
   - ***Fork-join tasks***. These are created via tarch::multicore::spawnTasks()
     which accepts a sequence of tasks. They are all run in parallel but then
     wait for each other, i.e. we define a tree (sub)task graph. With fork-join
     calls, we mirror the principles behind bulk-synchronous programming (BSP).

   ### Tasks with dependencies

   Task DAGs are built up along the task workflow. A task which is neither used
   within a fork-join region nor totally free identifies its dependencies
   through addresses: spawnTask() takes the address a task waits for and the
   address it in turn produces.

   Such a dependency is a sole completion dependency, i.e. it tells the task
   system which task has to be completed before the currently submitted one is
   allowed to start. A DAG is thereby built up layer by layer. We start with
   the first task, which may be executed immediately, and then work our way
   down through the graph adding node by node.

   In line with OpenMP and TBB, outgoing dependencies are declared before we
   use them.
   */
  namespace multicore {
    class Task;

    /**
     * Spawns a single task in a non-blocking fashion
     *
     * @see tarch::multicore and the section "Tasks with dependencies" therein
     *   for further documentation.
     *
     * @param task Pointer to a task. The responsibility for this task is
     *    handed over to the tasking system, i.e. you are not allowed to delete it.
     * @param inDependency Incoming data dependency that has to finish before
     *    the present task is allowed to run. You can pass nullptr to indicate no dependency.
     * @param outDependency Outgoing data dependency of the task.
     *    You can pass nullptr to indicate no dependency.
     */
    void spawnTask(Task* task, const void* const inDependency = nullptr, const void* const outDependency = nullptr);
    void spawnTasks(const std::vector<Task*>& tasks);

    void waitForTask(const void* const inDependency);
    void waitForAllTasks();

    namespace native {
      void spawnTask(Task* task);
      void spawnTask(Task* task, const void* const outDependency);
      void spawnTask(Task* task, const void* const inDependency, const void* const outDependency);
      void spawnTasks(const std::vector<Task*>& tasks);
      void waitForTask(const void* const inDependency);
      void waitForAllTasks();
    } // namespace native
  } // namespace multicore
} // namespace tarch
