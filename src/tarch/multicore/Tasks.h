// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>
#include <limits>
#include <list>
#include <string>
#include <tuple>
#include <vector>
#include <set>

#include "multicore.h"
#include "tarch/multicore/otter.h"
#include "tarch/multicore/orchestration/Strategy.h"

namespace tarch {
  namespace multicore {
    using TaskNumber = int;

    constexpr TaskNumber NoDependencies = -1;

    void setOrchestration(tarch::multicore::orchestration::Strategy* realisation);

    /**
     * Swap the active orchestration
     *
     * Different to setOrchestration(), this operation does not delete
     * the current orchestration. It swaps them, so you can use setOrchestration()
     * with the result afterwards and re-obtain the original strategy.
     */
    tarch::multicore::orchestration::Strategy* swapOrchestration(tarch::multicore::orchestration::Strategy* realisation
    );

    /**
     * Abstract super class for a job.
     */
    class Task {
    protected:
      const int _taskType;
      int       _priority;

    public:
      static constexpr int DefaultPriority = 1024;
      static constexpr int Host            = -1;
      static constexpr int DontFuse        = -1;

      /**
       * Construct task
       *
       * ## Task types and task numbers
       *
       * Eachtask in Peano has to have a task type. All tasks that do the same,
       * i.e. are of the same type, should have the same task type integer
       * marker. Peano's tarch can use multiple tasks of the same type and
       * fuse/batch them into one task call. However, if you don't want Peano
       * to even thing about task fusion, pass in DontFuse as argument for the
       * type.
       *
       * It is totally up to the user to manage the task type numbers. Peano
       * for example offers a factory mechanism peano4::parallel::getTaskType()
       * to create task types, which Peano applications should use. However,
       * the generation of the task types is not baked into the tarch.
       *
       * Some codes also have task numbers. They need this, if they have to
       * identify tasks uniquely. Applications for that are task dependencies
       * or tasks that are offloaded to other ranks. For the latter tasks, we
       * have to match data sent back by another rank to the task that would
       * have produced these results locally. Anyway, task numbers are not
       * baked into the generic interface, as we don't need them for all tasks
       * all the time, and I want to avoid that the construction of unique
       * task numbers becomes too expensive. If you should need unique task
       * numbers, I recommend you use reserveTaskNumber().
       *
       *
       * @param taskType Unique task (type) number for this task.
       *
       * @param priority Integer value that indicates what priority you want to
       *        assign a task. Value has to be non-negative.
       */
      Task(int taskType, int priority);

      virtual ~Task() {}

      int getTaskType() const;
      int getPriority() const;

      /**
       * Set priority
       *
       * @param priority Has to be non-negative
       */
      void setPriority(int priority);

      /**
       * @return true if the taskType is not DontFuse.
       */
      virtual bool canFuse() const;

      virtual bool run() = 0;

      /**
       * Fuse multiple tasks
       *
       * Fuse the task with a list of further tasks. The routine is guaranteed to
       * be called only for tasks with the same taskType. So if you carefully
       * distinguish your tasks, you can downcast all the arguments, as you might
       * know the real type.
       *
       * This operation is invoked on a task. However, it is also given N=otherTasks.size
       * further tasks of the same type. You have two options now:
       *
       * - You can process the N tasks in one rush. In this case, the original
       *   task, i.e. the object on which fuse() is called, remains intact and
       *   has not been processed yet. You have to return true.
       * - You can process the N+1 tasks in one rush, i.e. all tasks from
       *   otherTasks and the task represented by the actual object on which
       *   fuse is called. In this case, you return false, as the task on which
       *   fuse has been called can be destroyed straightaway by the runtime.
       *
       * No matter which route you follow, you always have to delete all the tasks
       * stores within otherTasks, as these have to be processed by fuse().
       *
       * <h2> Default implementation </h2>
       *
       * My default implementation executes all the passed tasks and then returns
       * the original task back, i.e. this one is not executed. This is the first
       * execution pattern described above.
       *
       * <h2> Memory ownership </h2>
       *
       * See above: fuse() has to delete all the instances within otherTasks.
       * The calling routine does not have to delete anything there anymore.
       * But it has to destroy the owning object, i.e. the object on which is
       * has called fuse(), manually no matter whether we return true or false.
       *
       * @return Is the present task still to be executed or can the runtime
       *         destroy it straightaway?
       * @param otherTasks List of tasks to fuse and process. Will all have the
       *         same type as the present object. It is the tasks responsibility
       *         to get these tasks done. So either span some new tasks or handle
       *         them straightaway.
       * @param targetDevice On which device should the task be processed? A
       *         negative number means local host anything greater or equal to
       *         zero denotes an accelerator.
       */
      virtual bool fuse(const std::list<Task*>& otherTasks, int targetDevice = Host);
    };

    /**
     * Helper class if you wanna administer tasks with in a queue
     *
     * It is a convenient class as it works both with real objects or
     * with pointers.
     */
    class TaskComparison {
    public:
      bool operator()(const Task& lhs, const Task& rhs) const;
      bool operator()(Task* lhs, Task* rhs) const;
    };

    /**
     * Frequently used implementation for job with a functor.
     */
    class TaskWithCopyOfFunctor: public Task {
    private:
      /**
       * See the outer class description for an explanation why this is an
       * attribute, i.e. why we copy the functor here always.
       */
      std::function<bool()> _taskFunctor;

    public:
      TaskWithCopyOfFunctor()                             = delete;
      TaskWithCopyOfFunctor(const TaskWithCopyOfFunctor&) = delete;
      TaskWithCopyOfFunctor(int taskType, int priority, const std::function<bool()>& taskFunctor);

      virtual bool run() override;
    };

    /**
     * Frequently used implementation for job with a functor.
     */
    class TaskWithoutCopyOfFunctor: public Task {
    private:
      /**
       * See the outer class description for an explanation why this is an
       * attribute, i.e. why we copy the functor here always.
       */
      std::function<bool()>& _taskFunctor;

    public:
      TaskWithoutCopyOfFunctor()                                = delete;
      TaskWithoutCopyOfFunctor(const TaskWithoutCopyOfFunctor&) = delete;

      TaskWithoutCopyOfFunctor(int taskType, int priority, std::function<bool()>& taskFunctor);

      virtual bool run() override;
    };

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
     */
    void spawnTask(
      Task* job,
      const std::set<TaskNumber>& inDependencies = std::set<TaskNumber>(),
      const TaskNumber&           taskNumber     = NoDependencies
    );

    /**
     * Wait for all tasks which have been spawned by spawnTask. This routine
     * might return and still miss out for a few pending tasks. It basically
     * runs only over those tasks with in/out dependencies and ensures that
     * they are either done or are pending.
     *
     */
    void waitForAllTasks();

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
       * These are the non-blocking (ready) tasks submitted via spawnTask()
       */
      int getNumberOfPendingTasks();

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
       *    equals NoDependencies, we know that noone will ever be able to wait
       *    for this task. In this case, we skip the storing.
       */
      void spawnTask(
        Task*                        job,
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
       * @return Global number of pending tasks in the pipeline
       */
      int getNumberOfWithholdPendingTasks();

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
      int mapPendingTasksOntoNativeTasks(int maxTasks);

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
      bool fusePendingTasks(const tarch::multicore::orchestration::Strategy::FuseInstruction& fuseInstruction);

      /**
       * Set up internal queues
       *
       * Set up the internal queues that our wrapper around the tasking back-end
       * requires. If we worked with only one task queue, this would not be
       * necessary, but we found such a solution detrimental. Instead, it is
       * way better to work with one queue per thread, so we can spawn enclave
       * tasks without any additional locks.
       *
       * This routine only evermakes the number of queues grow, never shrink.
       */
      void configureInternalTaskQueues(int numberOfTasks);

      /**
       * Commit local task queues into global queue
       *
       * If we work with thread-local queues, we have to flush them over into the
       * global queue eventually.
       *
       * We found that the commitment to the global queue is prone to starvation
       * effects: If we have 127 threads hammering the global queue for work and
       * if there's one thread left over, the one thread will struggle to get its
       * message into the global queue. This is particularly cumbersome, if the
       * local thread doesn't have any message.
       * I usually flush all queues if and only if I'm at the end of a BSP section.
       * You can however veto this behaviour.
       */
      void copyInternalTaskQueuesOverIntoGlobalQueue();

      void copyInternalTaskQueueOverIntoGlobalQueue(int threadNumber);
    } // namespace internal
  }   // namespace multicore
} // namespace tarch

bool operator<(const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs);
