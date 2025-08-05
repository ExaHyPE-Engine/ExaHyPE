// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#include <limits>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace tarch {
  /**
   @namespace tarch::multicore

   This page describes Peano 4's multithreading namespace. A more high-level
   overview is provided through @ref page_tarch_multicore.


   ## Writing your own code with multithreading features

   If you wanna distinguish in your code between multicore and no-multicore variants,
   please use

  ~~~~~~~~~~~~~~~~~~~~~~~~
  #include "tarch/multicore/multicore.h"
  ~~~~~~~~~~~~~~~~~~~~~~~~

  and

  ~~~~~~~~~~~~~~~~~~~~~~~~
  #if defined(WITH_MULTITHREADING)
  ~~~~~~~~~~~~~~~~~~~~~~~~

   With the symbol WITH_MULTITHREADING, you make your code independent of
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
     Each task can be assigned a unique number and incoming dependencies. The
     number in return can be used to specify outgoing dependencies.
   - ***Fork-join tasks***. These are created via tarch::multicore::spawnTasks()
     which accepts a sequence of tasks. They are all run in parallel but then
     wait for each other, i.e. we define a tree (sub)task graph. With fork-join
     calls, we mirror the principles behind bulk-synchronous programming (BSP).
   - ***Fusable tasks***. A subtype of the normal tasks.


   ### Tasks with dependencies

   In Peano, task DAGs are built up along the task workflow. That is, each task
   that is not used within a fork-join region or is totally free is assigned a
   unique number when we spawn it.

   Whenever we define a task, we can also define its dependencies. This is a sole
   completion dependency: you tell the task system which task has to be completed
   before the currently submitted one is allowed to start. A DAG thus can be built
   up layer by layer. We start with the first task. This task might be
   immediately executed - we do not care - and then we continue to work our way
   down through the graph adding node by node.

   In line with OpenMP and TBB - where we significantly influenced the development
   of the dynamic task API - outgoing dependencies should be declared before we
   use them.
   */
  namespace multicore {
    /**
     * Forward declaration
     */
    class Task;

    using TaskNumber                       = int;
    constexpr TaskNumber NoOutDependencies = -1;
    const auto           NoInDependencies  = std::set<TaskNumber>();

    /**
     * Spawns a single task in a non-blocking fashion
     *
     * Ownership goes over to Peano's multicore namespace, i.e. you don't have
     * to delete the pointer.
     *
     * ## Handling tasks without outgoing dependencies
     *
     * If outDependency equals nullptr, we know that no one is (directly)
     * waiting for this task, i.e. we won't add dependencies to the task
     * graph afterwards.
     *
     * spawnTask() will ***never*** commit a task to the global task
     * queue and therefore is inherently thread-safe.
     *
     * Spawn a task that may depend on other tasks' data.
     *
     * @see tarch::multicore and the section "Tasks with dependencies" therein
     *   for further documentation.
     *
     * @param task Pointer to a task. The responsibility for this task is
     *    handed over to the tasking system, i.e. you are not allowed to delete it.
     * @param inDependency Incoming data dependency that has to finish before
     *    the present task is allowed to run. You can pass nullptr to indicate no dependency.
     * @param outDependency Outcoming data dependency of the task.
     *    You can pass nullptr to indicate no dependency.
     */
    void spawnTask(
      Task*             task,
      const void* const inDependency = nullptr,
      const void* const outDependency = nullptr
    );
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
