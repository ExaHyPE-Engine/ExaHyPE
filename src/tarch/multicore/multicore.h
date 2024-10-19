// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include <string>
#include <set>
#include <vector>
#include <list>
#include <limits>

#include "config.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/multicore/orchestration/Strategy.h"

#if defined(SharedOMP) || defined(SharedTBB) || defined(SharedCPP) || defined(SharedSYCL)
#define SharedMemoryParallelisation
#endif

#pragma once

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
  #if defined(SharedMemoryParallelisation)
  ~~~~~~~~~~~~~~~~~~~~~~~~


   With the symbol SharedMemoryParallelisation, you make your code independent of
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


   ## Backends

   ### OpenMP

   If you want to use the OpenMP backend, you have to embed your whole main loop
   within an

  ~~~~~~~~~~~~~~~~~~~~~~~~~
  #pragma omp parallel
  #pragma omp single
  {
  ~~~~~~~~~~~~~~~~~~~~~~~~~

   environment. Furthermore, you will have to use

  ~~~~~~~~~~~~~~~~~~~~~~~~~
    export OMP_NESTED=true
  ~~~~~~~~~~~~~~~~~~~~~~~~~

   on some systems, as we rely heavily on nested parallelism.


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
   - ***Fork-join tasks***. These are created via tarch::multicore::spawnAndWait()
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


   ### Orchestration and (auto-)tuning

   The task orchestration is controlled via an implementation of
   tarch::multicore::orchestration::Strategy that you set via
   tarch::multicore::setOrchestration().
   With the strategy you can control the fusion of tasks and its shipping onto
   GPUs.
   Whenever we hit a fork-join section, i.e. encounter tarch::multicore::spawnAndWait(),
   you can also pick from different scheduling variants:

   1. Execute the tasks serially. Do not exploit any concurrency.
   2. Handle the tasks parallel. Any fusion is done within the runtime.
   3. Run through the forked tasks in parallel and afterwards fuse those tasks
      created manually. So you enforce when the fusion is happening.

   The orchestration of choice can control this behaviour via
   tarch::multicore::orchestration::Strategy::paralleliseForkJoinSection().

   */
  namespace multicore {
     /**
      * Forward declaration
      */
     class Task;

     using TaskNumber = int;

     constexpr TaskNumber NoOutDependencies = -1;

     const std::set<TaskNumber> NoInDependencies = std::set<TaskNumber>();

    /**
     * Switch on SmartMPI
     *
     * If you use SmartMPI, then the bookkeeping registers the the local scheduling.
     * If you don't use SmartMPI, this operation becomes nop, i.e. you can always
     * call it and configure will decide whether it does something useful.
     */
    void initSmartMPI();
    void shutdownSmartMPI();

    void setOrchestration(tarch::multicore::orchestration::Strategy* realisation);

    /**
     * Swap the active orchestration
     *
     * Different to setOrchestration(), this operation does not delete
     * the current orchestration. It swaps them, so you can use setOrchestration()
     * with the result afterwards and re-obtain the original strategy.
     */
    tarch::multicore::orchestration::Strategy* swapOrchestration(tarch::multicore::orchestration::Strategy* realisation);

    /**
     * Spawns a single task in a non-blocking fashion
     *
     * Ownership goes over to Peano's job namespace, i.e. you don't have
     * to delete the pointer.
     *
     * ## Handling tasks without outgoing dependencies
     *
     * If taskNumber equals NoDependency, we know that noone is (directly)
     * waiting for this task, i.e. we won't add dependencies to the task
     * graph afterwards. In this case, the realisation is straightforward:
     *
     * 1. If SmartMPI is enabled and the task should be sent away, do so.
     * 2. If the current orchestration strategy (an implementation of
     *    tarch::multicore::orchestration::Strategy says that we should
     *    hold back tasks, but the current number of tasks in the
     *    thread-local queue exceeds already this threshold, invoke
     *    the native tarch::multicore::native::spawnTask(task).
     * 3. If none of these ifs apply, enqueue the task in the thread-local
     *    queue.
     * 4. If we came through route (3), doublecheck if we should fuse
     *    tasks into GPUs.
     *
     * spawnTask() will ***never*** commit a task to the global task
     * queue and therefore is inherently thread-safe.
     *
     * ## Tasks with a task number and incoming dependencies
     *
     * Spawn a task that depends on one other task. Alternatively, pass in
     * NoDependency. In this case, the task can kick off immediately. You have
     * to specify a task number. This number allows other, follow-up tasks to
     * become dependent on this very task. Please note that the tasks have to
     * be spawned in order, i.e. if B depends on A, then A has to be spawned
     * before B. Otherwise, you introduce a so-called anti-dependency. This is
     * OpenMP jargon which we adopted ruthlessly.
     *
     * You may pass NoDependency as taskNumber. In this case, you have a
     * fire-and-forget task which is just pushed out there without anybody
     * ever waiting for it later on (at least not via task dependencies).
     *
     *
     * @see tarch::multicore and the section "Tasks with dependencies" therein
     *   for further documentation.
     * @see tarch::multicore::spawnAndWait() for details what happens with
     *   tasks that have no outgoing dependencies.
     * @see processPendingTasks(int) describing how we handle pending tasks.
     *
     * @param task Pointer to a task. The responsibility for this task is
     *   handed over to the tasking system, i.e. you are not allowed to delete
     *   it.
     * @param inDependencies Set of incoming tasks that have to finish before
     *   the present task is allowed to run. You can pass the alias
     *   tarch::multicore::Tasks::NoInDependencies to make clear what's
     *   going on.
     * @param taskNumber Allow the runtime to track out dependencies. Only
     *   numbers handed in here may be in inDependencies in an upcoming call.
     *   If you do not expect to construct any follow-up in-dependencies, you
     *   can pass in the default, i.e. NoOutDependencies.
     */
    void spawnTask(
      Task*                        task,
      const std::set<TaskNumber>&  inDependencies = tarch::multicore::NoInDependencies,
      const TaskNumber&            taskNumber = tarch::multicore::NoOutDependencies
    );

    /**
     * Wait for set of tasks
     *
     * Entries in inDependencies can be NoDependency. This is a trivial
     * implementation, as we basically run through each task in
     * inDependencies and invoke waitForTask() for it. We don't have to rely on
     * some backend-specific implementation.
     *
     * ## Serial code
     *
     * This routine degenerates to nop, as no task can be pending. spawnTask()
     * always executed the task straightaway.
     */
    void waitForTasks(const std::set<TaskNumber>& inDependencies);

    /**
     * Wrapper around waitForTasks() with a single-element set.
     */
    void waitForTask(const int taskNumber);

    /**
     * Fork-join task submission pattern
     *
     * The realisation is relatively straightforward:
     *
     * - Maintain nestedSpawnAndWaits which is incremented for every fork-join
     *   section that we enter.
     * - Tell the orchestration that a BSP section starts.
     * - Ask the orchestration which realisation to pick.
     * - Either run through the task set sequentially or invoke the native
     *   parallel implementation.
     * - If there are task pending and the orchestration instructs us to do so,
     *   map them onto native tasks.
     * - Tell the orchestration that the BSP section has terminated
     * - Tell the orchestration that a BSP section ends.
     * - Maintain nestedSpawnAndWaits which is decremented whenever we leave a
     *   fork-join section.
     *
     *
     * ## Scheduling variants
     *
     * The precise behaviour of the implementation is controlled through the
     * orchestration. At the moment, we support three different variants:
     *
     * 1. The serial variant tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially runs
     *    through all the tasks one by one. Our rationale is that a good
     *    orchestration picks this variant for very small task sets where the
     *    overhead of the join-fork makes a parallelisation counterproductive.
     *
     * 2. The parallel variant tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel runs
     *    through all the tasks in parallel. Once all tasks are completed, the
     *    code commits all the further tasks that have been spawned into a
     *    global queue and then studies if to fuse them further or if to map
     *    them onto native tasks. This behaviour has to be studied in the
     *    context of tarch::multicore::spawnTask() which might already have
     *    mapped tasks onto native tasks or GPU tasks, i.e. at this point no
     *    free subtasks might be left over in the local queues even though
     *    there had been some. It is important to be careful with this "commit
     *    all tasks after the traversal" approach: In OpenMP, it can lead to
     *    deadlocks if the taskwait is realised via busy polling. See the bug
     *    description below.
     *
     * 3. The parallel variant
     * tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks runs through all
     * the tasks in parallel. Different to tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel, it
     * does not try to commit any further subtasks or to fuse them. This variant allows the scheduler to run task sets
     * in parallel but to avoid the overhead introduced by the postprocessing.
     *
     * I would appreciate if we could distinguish busy polling from task
     * scheduling in the taskwait, but such a feature is not available within
     * OpenMP, and we haven't studied TBB in this context yet.
     *
     *
     * ## Implementation flaws in OpenMP and bugs burried within the sketch
     *
     * In OpenMP, the taskwait pragma allows the scheduler to process other
     * tasks as it is a scheduling point. This way, it should keep cores busy
     * all the time as long as there are enough tasks in the system. If a
     * fork-join task spawns a lot of additional subtasks, and if the
     * orchestration does not tell Peano to hold them back, the OpenMP runtime
     * might switch to the free tasks rather than continue with the actual
     * fork-join tasks. Which is not what we want and introduces runtime flaws
     * later down the line. This phenomenon is described in our
     * 2021 IWOMP paper by H. Schulz et al.
     *
     * A more severe problem arises the other way round: Several groups have
     * reported that the taskwait does not continue with other tasks. See in
     * particular
     *
     * Jones, Christopher Duncan (Fermilab): Using OpenMP for HEP Framework Algorithm Scheduling.
     * http://cds.cern.ch/record/2712271
     *
     * Their presentation slides can be found at https://zenodo.org/record/3598796#.X6eVv8fgqV4.
     *
     * This paper clarifies that some OpenMP runtimes do (busy) waits within
     * the taskwait construct to be able to continue immediately. They do not
     * process other tasks meanwhile. Our own ExaHyPE 2 POP review came to the
     * same conclusion.
     *
     * This can lead to a deadlock in applications such as ExaHyPE which spawn
     * bursts of enclave tasks and then later on wait for their results to drop
     * in. The consuming tasks will issue a taskyield() but this will not help,
     * if the taskyield() now permutes through all the other traversal tasks.
     *
     * If you suffer from that, you have to ensure that all enclave tasks
     * have finished prior to the next traversal.
     *
     *
     * ## Statistics
     *
     * It is important to know how many BSP sections are active at a point. I
     * therefore use the stats interface to maintain the BSP counters. However,
     * I disable any statistics sampling, so I get a spot-on overview of the
     * number of forked subtasks at any point.
     *
     *
     *
     * @todo Speak to OpenMP. It would be totally great, if we could say that
     *   the task wait shall not(!) issue a new scheduling point. We would like
     *   to distinguish taskwaits which priorities throughput vs algorithmic
     *   latency.
     *
     * @todo Speak to OpenMP that we would like a taskyield() which does not (!)
     *   continue with a sibling. This is important for producer-consumer
     *   patterns.
     */
    void spawnAndWait(const std::vector<Task*>& tasks);


    namespace native {
      /**
       * Map onto native tasking
       *
       * Run over the tasks and issue native tasks. If the tasks spawn, in
       * return, further subtasks, these will either end up in a thread-local
       * queue or will be mapped onto native tasks. The behaviour here depends
       * on decisions within tarch::multicore::spawnTask() guided by the
       * orchestration. If the subtasks enqueue tasks into the thread-local
       * queue, they will remain there. This routine does not touch the
       * thread-local queue.
       *
       * The responsibility for the pointers in tasks is handed over to the
       * runtime of choice, i.e. you don't have to delete them.
       *
       * @param tasks Set of tasks. Is guaranteed to be non-empty.
       */
      void spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks);

      /**
       * Process a fused task
       *
       * A fused task is a task (which can be fused) and a list of further
       * tasks which are of the same type and, hence, can be fused, too. The
       * list can be empty. Efficient implementations should spawn the fused
       * tasks as a further ready task and return immediately.
       *
       * ## No multithreading
       *
       * If no multithreading is enabled, then we map it onto a plain task via
       * a spawnTask(). This spawn has no dependencies.
       *
       *
       *
       * @param firstTask First task. This is a task which can be fused. The pointer
       *   is valid. The ownership of firstTask is handed over to the called routine,
       *   i.e. processFusedTask() has to ensure that it is deleted.
       * @param otherTasks List of tasks of the same type. The list can be empty.
       *   processFusedTask() has to ensure that all tasks stored within the list are
       *   executed and subsequently destroyed.
       * @param device Target device on which the fused tasks should be executed. Can
       *   be host if the tasks should end up on the host.
       */
      void processFusedTask(Task* firstTask, const std::list<tarch::multicore::Task*>& otherTasks, int device);

      /**
       * Spawn a new task into the tasking backend
       *
       * This routine is invoked by tarch::multicore::spawnTask(). The invoking
       * method is not a sole forwarding. It takes care of the categorisation
       * of ready tasks into pending tasks, and it also handles all the fusion
       * for GPUs. Therefore, if it delegates calls to this class, we really
       * can directly map the task 1:1 onto the native task system used.
       *
       *
       * ## Serial code
       *
       * This is a trivial case in a serial run: As we have already completed
       * all incoming tasks (we work serially and all incoming tasks have to
       * be done by this time), we can simply process the task and return. As a
       * direct consequence, all dependencies of future tasks are always
       * fulfilled.
       *
       *
       * ## C++ variant
       *
       * 1. We first check if a task with this number is among the submitted
       *    tasks. This is possible if a previous algorithm phase has submitted
       *    a task with this number which was a sink, but noone ever waited for
       *    it. This should usually not happen and is a sign of bad code
       *    design. We issue a warning, free all locks and wait for the
       *    previous task with the same number to terminate.
       * 2. Next, we spawn the actual task via a std::async call. The core task
       *    code consists of three parts:
       *    - We first run through all the dependencies and ensure they all are
       *      fulfilled. waitForTasks() realises check.
       *    - Execute the task object.
       *    - Free its memory.
       * 3. We store the shared_future returned by the task. If the task number
       *    equals NoOutDependencies, we know that noone will ever be able to wait
       *    for this task. In this case, we skip the storing.
       *
       *
       * ## OpenMP
       *
       * In OpenMP, we have less explicit control over the number of alive
       * tasks compared to C++ for example. However, nothing stops us from
       * checking if an in-dependency has been fulfilled. If this in-dependency
       * does not exist, then we have an anti-dependency. But this one does not
       * matter. It will just be executed sequentially.
       *
       * OpenMP models dependencies through addresses. While it does not use
       * the data to which these addresses point to, it is important that the
       * addresses are valid, i.e. the system owns these memory locations.
       * I use an internal array to administer the byte array which I
       * use for dependencies. See the description of the internal variable
       * memoryRegionToTrackTaskDependencies and notably
       * canServeDependencyEntry() for a discussion of the required
       * dynamic growth.
       *
       * OpenMP now has iterators in taskwait and dependency statements. Yet,
       * the set of in-dependencies is not iterable. So we create a vector into
       * which we paste the task numbers. This also contains the outgoing task
       * number, i.e. if there had been a task with that number before, we
       * basically impose an inout dependency and ensure this one terminates
       * first.
       *
       * ### Realisation pitfalls
       *
       * I originally had
       *
       * ~~~~~~~~~~~~~~~~~~~~~
  #pragma omp atomic
  numberOfSpawnedTasks++;
         ~~~~~~~~~~~~~~~~~~~~~
       *
       * outside of the if/else cascade. However, that means that the
       * subsequent canServeDependencyEntry() calls will not increase the
       * memory used. They assume that the task were already out. Therefore,
       * it is important to increment this counter as late as possible.
       *
       *
       * ### Task dependencies
       *
       * OpenMP now supports iterators and accessors over arrays. I tried
       *
       * ~~~~~~~~~~~~~~~~~~~~~
      DependencyTrackingDataType* outDependencyPointer = dependencyTrackingBaseAddress + taskNumber;
      [...]
      #pragma omp task [...] depend(out: *outDependencyPointer) untied
         ~~~~~~~~~~~~~~~~~~~~~
       *
       * but that didn't work. So it seems that OpenMP struggles with the
       * dereferencing. However, it seems that
       *
       * ~~~~~~~~~~~~~~~~~~~~~
      #pragma omp task [...] depend(out: dependencyTrackingBaseAddress[taskNumber]) untied
         ~~~~~~~~~~~~~~~~~~~~~
       *
       * does work.
       *
       *
       * ## No multithreading
       *
       * We just execute this task. We know that tasks may not have anti-dependencies.
       * That is, we know that any incoming task has been spawned before. By
       * induction, it therefore has already completed. We also don't have any
       * competition, as there are no competing tasks. So we actually can ignore
       * all the parameters and just execute the task.
       */
      void spawnTask(
        Task*                        task,
        const std::set<TaskNumber>&  inDependencies,
        const TaskNumber&            taskNumber
      );

      /**
       * Wait for other task
       *
       *
       * ## Serial code
       *
       * Degenerates to nop here, as all incoming tasks are completed. We just
       * have to ensure that the task we are waiting for is not(!) registered,
       * i.e. has actually been submitted.
       *
       * ## C++ code
       *
       * waitForTasks() is quite tricky, as we have to avoid any deadlocks
       * here. We therefore use a quite defensive programming.
       *
       * 1. We lock the set of spawned tasks. If a dependency task is in there
       *    and is valid, we memorise it in a pointer checkFuture and release
       *    the lock.
       * 2. Should we have found a dependency, we call the C++ wait. From
       *    hereon, the core task is complete, but there's still an entry in
       *    the global lookup table of spawned tasks. At this point, we
       *    might actually have finished a lot of tasks recursively.
       * 3. We briefly lock the data structure again and check if the task is
       *    bookmarked in there and is already complete. In this case, we
       *    remove the entry. We kind of run an on-the-fly immediate garbage
       *    collection to slim down our map of spawned tasks.
       */
      void waitForTasks(const std::set<TaskNumber>& inDependencies);

      /**
       * Slightly different than the umbrella version in the general namespace.
       * See comments there. This version only waits for tasks which have been
       * submitted with a valid task number.
       *
       * ## Serial code
       *
       * This routine degenerates to nop, as no task can be pending. spawnTask()
       * always executed the task straightaway.
       *
       * ## C++
       *
       * There are different ways how to implement this wait: The most
       * straight-forward implementation takes all the keys and the invokes
       * waitForTasks() for all keys. We do not implement this pattern, as it
       * would mean that we potentially copy quite a lot of keys. Instead, we
       * iteratively wait for one task after another to finish. This implies
       * that we give the runtime system a lot of freedom in which order and
       * how parallel to tidy up all tasks.
       */
      void waitForAllTasks();
    } // namespace native

    /**
     * Process a few tasks from my backlog of tasks that could be fused
     *
     * Peano buffers tasks that can be fused in a separate buffer. With this
     * routine, you can ensure that this buffer is emptied.
     *
     * This routine is rarely used directly. If you want the runtime to fuse
     * the tasks at specific points, it is usually better to implement your
     * own orchestration strategy and to let Strategy::fuse() return an
     * instruction that tells Peano to fuse once a certain fork-join region
     * has closed.
     *
     * Delegates to tarch::multicore::taskfusion::processPendingTasksOnHost()
     */
    void waitForAllTasks();
  } // namespace multicore
} // namespace tarch
