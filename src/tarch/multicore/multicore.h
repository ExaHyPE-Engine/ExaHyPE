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


   ## Documentation

   The core documentation of Peano's multicore layer is part of the generic
   @ref page_tarch_multicore documentation.

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

    tarch::multicore::orchestration::Strategy& getOrchestration();

    /**
     * Spawns a single task in a non-blocking fashion
     *
     * Ownership goes over to Peano's job namespace, i.e. you don't have
     * to delete the pointer.
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
     * ## Usage with functors
     *
     * If you have a task object, you can directly call it. If you have only a
     * functor, use tarch::multicore::TaskWithCopyOfFunctor.
     *
     *
     * ## OpenMP variant
     *
     * The OpenMP variant has to jump through a number of hoops:
     *
     * 1. We check if we have enough addresses to administer the task
     *    dependencies. We basically ask: should we grow this big table
     *    that holds the dependencies (dependencyTrackingBaseAddress).
     * 2. If that analysis says "yes", we acquire a write lock on the
     *    whole tasking. Otherwise, we acquire a read lock over a
     *    MultiReadSingleWriteSemaphore instance.
     * 3. We grow the task dependency management memory region if we
     *    have got a write lock.
     * 4. We look for each involved task number if this can be modelled
     *    via an OpenMP dependency, i.e. if the task number is one that
     *    we can manage. Otherwise, we move this dependency (we know
     *    that the number of tasks managed is monotonously growing, i.e.
     *    we can assume that this dependency's task has never been used).
     * 5. If there are no out dependencies, we can just spawn this task.
     *    It goes straight into the background.
     * 6. If we have an out dependency (task number) and if we can serve
     *    this one, we spawn a proper task.
     * 7. Otherwise, we complete the task immediately.
     *
     * @see The page @ref page_tarch_multicore contains some high level
     *      overview of the multithreading.
     * @see The documentation of tarch::multicore::taskfusion for details
     *      how we handle task fusion.
     * @see tarch::multicore::taskfusion::handleFusableTask()
     *      for details how we react if task->canFuse() holds.
     * @see tarch::native::spawnTask() for implementation details if a task
     *      cannot be fused.
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
     * This is a hint type which can be used to accelerate any taskwaits
     */
    enum class TaskWaitType {
      Mixed,
      ExclusivelyFusedTasks,
      OnlyNativeTasks
    };

    /**
     * Wait for set of tasks
     *
     * Entries in inDependencies can be NoDependency. This is a trivial
     * implementation, as we basically run through each task in
     * inDependencies and invoke waitForTask() for it. We don't have to rely on
     * some backend-specific implementation.
     *
     * ## Usage
     *
     * You can obviously construct a task set explicitly. If you know the
     * number of tasks, you can however directly use the bracket notation
     * to invoke this function:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *    int taskNumber = ...;
     *    tarch::multicore::waitForTasks({taskNumber+0,taskNumber+1});
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * ## Serial code
     *
     * This routine degenerates to nop, as no task can be pending. spawnTask()
     * always executed the task straightaway.
     *
     *
     *
     * ## Optimisation
     *
     * If you know that you have exclusively native tasks, then we can skip
     * the check if there are any fused tasks still pending that we have to map
     * onto native tasks first.
     *
     * If you have exclusively fused tasks, then we can hand the baton on to
     * the fusion subpackage, but in return ask it if there's a chance that
     * some native tasks arise from the fusion. This is not the case for all
     * fusion strategies. If so, we can skip the subsequent test for native
     * tasks. See tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks().
     *
     *
     * @param taskWaitType This is an optimisation hint. See description above.
     */
    void waitForTasks(const std::set<TaskNumber>& inDependencies, TaskWaitType taskWaitType = TaskWaitType::Mixed);

    /**
     * Wrapper around waitForTasks() with a single-element set.
     */
    void waitForTask(const int taskNumber, TaskWaitType taskWaitType = TaskWaitType::Mixed);

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
       * ## TBB
       *
       * With TBB, we maintain two versions: one for a new (preview) TBB
       * version where dynamic task graph construction is supported, and one
       * for an older version, where such a feature is not yet available, and
       * we need our own TBB extension.
       *
       * In the one version, we create a task handle via a defer() call. In
       * the other version, we create an instance of dynamic_task_graph_node.
       * If the task has no outgoing dependencies, we don't have to register
       * it in some way, i.e. to memorise, as noone will be able to refer to
       * it later anyway. However, we have to add an additional incoming
       * dependency in case that such a task has existed before.
       *
       * Next, we add the user-defined incoming dependencies to this task.
       *
       * Finally, we either move the handle into the global task group, making
       * it a proper task (new TBB version), or we put it into our own task
       * graph object which then translates it into a proper task.
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
       *
       * ## OpenMP
       *
       * Joachim Jenke has flagged that there are some OpenMP versions that
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * #pragma omp taskwait depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]])
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * might fail with some (older) OpenMP versions. A workaround is the
       * construct through an empty task:
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * #pragma omp task depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]]) if (0)
       * {}
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * Here, we model a task which is actually never spawned as task due to
       * if statement that evaluates to false. However, the in-dependencies
       * will be resolved nevertheless.
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
       *
       * ## OpenMP
       *
       * In OpenMP, we do a generic taskwait, but then we use the opportunity
       * as well to grow the memory used for dependency tracking by calling
       * reserveDependencyTrackingMemory(). We still need the generic taskwait
       * first, as there's no guarantee that reserveDependencyTrackingMemory()
       * will actually expand the memory and wait for all other tasks.
       */
      void waitForAllTasks();
    } // namespace native


    /**
     * Wait for all tasks notably has to take fused tasks into account.
     */
    void waitForAllTasks();


    /**
     * Process a fused task
     *
     * A fused task is a task (which can be fused) and a list of further
     * tasks which are of the same type and, hence, can be fused, too. This
     * routine has to process all tasks, delete all the instances, i.e. both
     * myTask and all pointers stored in tasksOfSameType, and return. It is
     * blocking in the sense that we are guaranteed that tasks have completed
     * once this function returns. For the execution, we rely on myTask's
     * fuse() operation, i.e. this routine is solely there for the memory
     * management.
     *
     * ## Generic efficiency remarks
     *
     * It is generally not clear if it is better to spawn all tasks again as
     * a big parallel for, or to process them sequentially without any further
     * tasking. After all, the task fusion has been introduced to reduce task
     * management overhead. If we now take a set of tasks and again map them
     * onto a set of physical tasks, it is really not clear what we gain in
     * the end, as we again introduce overhead.
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
    void processFusedTask(Task* myTask, const std::vector<tarch::multicore::Task*>& tasksOfSameType, int device);

  } // namespace multicore
} // namespace tarch
