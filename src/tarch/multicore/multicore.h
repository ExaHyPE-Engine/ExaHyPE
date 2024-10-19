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

 If the Peano statistics are enabled, the tasking back-end will sample some
 quantities such as

 - "tarch::multicore::bsp-concurrency-level" Typically corresponds to the
   number of fork-join traversal tasks.
 - "tarch::multicore::global-pending-tasks" Global pending tasks.
 - "tarch::multicore::thread-local-pending-tasks" Pending tasks per thread
   which are not yet committed to the global queue.
 - "tarch::multicore::fuse-tasks" Number of tasks which have been fused.

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
 - Free floating tasks (task sinks). Tasks without any outgoing dependencies
   are free floating in the sense that we never wait for them. That's obviously
   almost never true - few tasks have no follow-up dependencies at all - but
   the user might decide to model the out dependencies without task
   dependencies: Typically, such tasks set some output flag or dump their
   result into a database, which then in turn unlocks follow-up tasks.
 - Pending tasks: Pending tasks are ready tasks which Peano's tasking API
   holds back on purpose. They are not (yet) submitted to the tasking
   system, as we might want to fuse them into larger task assemblies (and
   move them to other ranks/devices, e.g.).


 ## Vanilla Peano task graph

 The two routines allow us to model the typical Peano 4 task graph:

 @image html multicore_task_flow.png

 Peano's main core realises a classic fork-join parallelism (left) where the
 fork-join segments are realised via tasks. The routine tarch::multicore::spawnAndWait()
 is used to construct this task graph part on-the-fly. Formally, this part of the
 code is very simple to classic BSP (cmp OpenMP's parallel for), which is recursively
 nested into each other.

 Each task within the core fork-join DAG might spawn additional tasks.
 When we wait for the BSP part of the graph (left) to terminate, these tasks
 still might linger around. They should backfill any empty task queue when it
 is appropriate or, in general, be executed with low priority. Late on
 throughout the execution, some further tasks for the core code section
 will need the outcome of tasks that have been spawned before. At this
 point, the tasks should already be completed (and therefore have dumped
 their outcomes into a database). Otherwise, we have to manually invoke
 the scheduler manually via tarch::multicore::processPendingTasks().


 The tasks to the right are typically called enclave tasks, and they are
 typically added on top of the core fork-join task structure by Peano
 extensions. The prime example is ExaHyPE 2. Enclave tasking yields real bursts
 of low priority tasks, and it would be unacceptable complicated to model the
 dependencies on their outcomes via a task graph. Therefore, I went down the
 route that these tasks dump their outcomes into a table, and consumer tasks
 (or receivers in C++ terminology) then take the outcomes from there.

 There is a further reason to work with the dedicated queues: As we found out
 and describe in the 2021 IWOMP paper by H. Schulz et al, tasking frameworks
 such as OpenMP might decide to switch to any ready task throughout the
 execution. This means that they might postpone the processing of the
 DAG to the left and instead do the free tasks on the right. This is not
 what we want: We want them to have lower priority. An important paper to read
 in the context of the present solution is also
 https://www.osti.gov/pages/servlets/purl/1465188. Note in particular the
 example in Figure 3.


 ### GPUs and an additional queue (pending tasks)

 On some platforms and code generations, it has proven of value to add an
 additional runtime (layer) on top of the native tasking backend. These layers
 do exist for TBB and OpenMP, e.g., and all follow the same general pattern.
 Basically, they introduce an additional, user-managed task queue on top of the
 native tasking systems. Enclave tasks are not spawned into the native runtime
 but instead held back within this additional queue. There are two advantages
 of this:

 1. Some runtimes see the task spawning as scheduling point (OpenMP e.g.) and
    might decide to run the enclave tasks immediately, as there's so many of
    them. However, we really want to have them low priority. By holding them
    back in our own queue, they are invisible to the tasking runtime and
    therefore cannot be scheduled.

 2. We can search the additional queue of tasks of the same type which can fit
    to the GPU. If we find multiple of these guys, we pack them together into
    one large meta task (or task assembly) and throw them onto an accelerator.

 Performance analysis shows that using one global queue on top of the actual
 tasking backend quickly introduces performance issues, as too many tasks
 might try to spawn their tasks concurrently into this queue. We suffer from
 congestion. Therefore, the implementation in Tasks.cpp employs one global
 queue plus a queue per thread.


 ### Tasks with dependencies

 In Peano, task DAGs are built up along the task workflow. That is, each task
 that is not used within a fork-join region or is totally free is assigned a
 unique number when we spawn it.

 Whenever we define a task, we can also define its dependencies. This is a sole
 completion dependency: you tell the task system which task has to be completed
 before the currently submitted one is allwed to start. A DAG thus can be built
 up layer by layer. We start with the first task. This task might be
 immediately executed - we do not care - and then we continue to work our way
 down through the graph adding node by node.

 Different to OpenMP, outgoing dependencies do not have to be declared. We
 solely model in dependencies. This however imples that all predecessors of a
 task have to be submitted before we add this very task. This is not always
 possible. When you walk top-down through a tree and then bottom-up again,
 you might have thrown away finer levels when you get back to the original
 one and thus you are unable to introduce dependencies from fine to coarse
 unless you introduce very fancy bookkeeping.

 We avoid such bookkeeping (or deploy it into the tasking API), as we allow
 codes to register a task. registerTask() tells the system "there will be a
 task eventually, but I haven't yet constructed it". You can also add
 dependencies from an existing task to such a registered task. Once you
 finally submit the registered task, you are however not allowed to add any
 dependencies anymore.



 ### Orchestration and (auto-)tuning

 The actual orchestration is controlled via an implementation of
 tarch::multicore::orchestration::Strategy that you set via
 tarch::multicore::setOrchestration(). The strategy basically controls:

 - How many enclave tasks should be hold back in the user-defined queue. If
   you exceed this treshold, the backend maps each enclave task 1:1 onto a
   native task.
 - How many tasks should the system try to fuse into one GPU call.
 - Should the code try to fuse these tasks immediately when they are spawned
   or search for fusion candidates every time a fork-join section has
   terminated.
 - What is an appropriate scheduling strategy for a fork-join section depending
   on the level of nested parallelisation calls.

 The class tarch::multicore::orchestration::StrategyFactory allows you to pick
 various common strategies, and it also provides the routine which determines
 the default variant which is chosen if the user does not manually pick one.
 As we outsource the orchestration into strategy objects, users can implement
 online and offline autotuning within through an implementation of
 tarch::multicore::orchestration::Strategy.


 ### Scheduling flavours for fork-join sections

 Whenever we hit a fork-join section, i.e. encounter tarch::multicore::spawnAndWait(),
 there are different scheduling variants on the table:

 1. Execute the tasks straightaway. Do not exploit any concurrency.
 2. Run a straightforward, native task loop.
 3. Run through the forked tasks in parallel and check eventually if we should
    map some subtasks onto native tasks.

 The orchestration of choice can control this behaviour via
 tarch::multicore::orchestration::Strategy::paralleliseForkJoinSection().

 The documentation of tarch::multicore::spawnAndWait()
 provides rationale why I think that variant (2) minimises the algorithmic
 latency, whereas variant (3) maximises occupation, and



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

    namespace internal {
      /**
       * Take up to maxTasks tasks that are internally buffered and throw them
       * into the native runtime
       *
       * This routine should submit the tasks LIFO, as we assume that Peano's
       * traversals produce tasks and the follow-up traversal
       * then consumes the outcome. In the Peano world, two subsequent mesh
       * traversals run through the grid in inverted order, so submitting tasks
       * LIFO makes sense once we assume that most runtimes will kind of stick
       * to the task submission order unless there are dependencies between the
       * tasks.
       */
      int mapPendingTasksOntoNativeTasks();

      /**
       *
       * The routine searches through the global task queue and tries to
       * fuse tasks. Therefore, calling the routine is kind of pointless
       * if you haven't committed the tasks to the global queue yet. In
       * all places where you invoke the routine, it is thus important
       * that you've committed any thread-local tasks to the global queue
       * if you think that fusion should be successful.
       *
       * # Thread safety
       *
       * The method locks and unlocks the queue. So don't lock it yourself.
       *
       */
      bool fusePendingTasks();
    } // namespace internal

    /**
     * Process a few tasks from my backlog of tasks
     *
     * This routine tries to complete maxTasks. It is important that this routine
     * makes progress, i.e. processes tasks, if there are any tasks left in the
     * system. ExaHyPE's enclave tasking, for example, uses the process within
     * its polling for enclave task results. If it does not make progress, this
     * routine will starve. As such, it is absolutely fine if the implementation
     * of processPendingTasks() suspends the actual thread, as long as this thread
     * is not permanently suspended.
     *
     * This routine invokes internal::copyInternalTaskQueuesOverIntoGlobalQueue()
     * first of all to maximise the number of tasks in the local queue.
     *
     *
     * @param maxTasks Specify how many tasks to process at most. By constraining
     *  this number, you can realise some polling where you check for a condition.
     *  If the condition is not met, you ask the task system to complete a few
     *  tasks, but you don't want the task system to complete all tasks, as you
     *  don't want to wait for ages before you check again.
     *
     * @param fifo shall the system try to complete the tasks in FIFO order? This
     *  is a recommendation. Not all task processing strategies do support such a
     *  clue mechanism.
     *
     * @return There have been tasks
     */
    bool processPendingTasks(int maxTasks = std::numeric_limits<int>::max(), bool fifo = true);
  } // namespace multicore
} // namespace tarch
