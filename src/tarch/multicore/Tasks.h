// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "multicore.h"
#include "tarch/multicore/orchestration/Strategy.h"


#include <functional>
#include <vector>
#include <list>
#include <string>
#include <limits>


namespace tarch {
  namespace multicore {
    void setOrchestration( tarch::multicore::orchestration::Strategy* realisation );

    /**
     * Swap the active orchestration
     *
     * Different to setOrchestration(), this operation does not delete
     * the current orchestration. It swaps them, so you can use setOrchestration()
     * with the result afterwards and re-obtain the original strategy.
     */
    tarch::multicore::orchestration::Strategy* swapOrchestration( tarch::multicore::orchestration::Strategy* realisation );

    /**
     * Usually called directly by EnclaveTask.
     *
     * This routine has to return a number which is currently not in use.
     * There is however no need for the delivered task numbers to be
     * consecutive. Therefore, I use the size of the existing task set as
     * a guideline for a task number to search for. If this initial guess
     * is already handed out, I increase the counter by a prime number and
     * try the new number again - until I've finally found a task which is
     * not yet processed.
     */
    int reserveTaskNumber();

    void releaseTaskNumber(int number);

    /**
     * Should usually be more than pending tasks, as it also includes tasks that have
     * finished yet have not released their number yet.
     *
     * @return Number of tasks that are currently reserved. I assume that these belong
     *  to tasks that are either running or that have produced outcome that's not yet
     *  used.
     */
    int getNumberOfReservedTaskNumbers();

    /**
     * Abstract super class for a job.
     */
    class Task {
      protected:
        const int   _taskType;
        int         _priority;
      public:
        static constexpr int DefaultPriority = 0;
        static constexpr int Host            = -1;
        static constexpr int DontFuse        = -1;

        /**
         * @param taskType Unique task (type) number for this task. All tasks that do 
         *        the same, i.e. are of the same type, should have the same task type
         *        integer marker. Peano's tarch can use multiple tasks of the same
         *        type and fuse/batch them into one task call. However, if you don't
         *        want Peano to even thing about task fusion, pass in DontFuse as
         *        argument.
         * @param priority Integer value that indicates what priority you want to
         *        assign a task. Not all tasking backends support priorities.
         */
        Task( int taskType, int priority );

        virtual ~Task() {}

        int getTaskType() const;
        int getPriority() const;
        void setPriority( int priority );

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
        virtual bool fuse( const std::list<Task*>& otherTasks, int targetDevice=Host );
    };


    /**
     * Helper class if you wanna administer tasks with in a queue
     *
     * It is a convenient class as it works both with real objects or
     * with pointers.
     */
    class TaskComparison {
      public:
        bool operator() (const Task& lhs, const Task& rhs) const;
        bool operator() (Task* lhs, Task* rhs) const;
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
        std::function<bool()>   _taskFunctor;
      public:
        TaskWithCopyOfFunctor() = delete;
        TaskWithCopyOfFunctor( const TaskWithCopyOfFunctor& ) = delete;
        TaskWithCopyOfFunctor( int taskType, int priority, const std::function<bool()>& taskFunctor );

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
        std::function<bool()>&   _taskFunctor;
      public:
        TaskWithoutCopyOfFunctor() = delete;
        TaskWithoutCopyOfFunctor( const TaskWithoutCopyOfFunctor& ) = delete;

        TaskWithoutCopyOfFunctor( int taskType, int priority, std::function<bool()>& taskFunctor );

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
    bool processPendingTasks(int maxTasks = std::numeric_limits<int>::max(), bool fifo=true);


    /**
     * Spawns a single task in a non-blocking fashion
     *
     * Ownership goes over to Peano's job namespace, i.e. you don't have
     * to delete the pointer.
     *
     * The realisation is straightforward:
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
     * @see tarch::multicore::spawnAndWait()
     * @see processPendingTasks(int)
     *
     */
    void spawnTask(Task*  job);


    /**
     * Run a bunch of tasks
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
     * 3. The parallel variant tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks runs
     *    through all the tasks in parallel. Different to tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel,
     *    it does not try to commit any further subtasks or to fuse them. This
     *    variant allows the scheduler to run task sets in parallel but to
     *    avoid the overhead introduced by the postprocessing.
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
     * Jones, Christopher Duncan (Fermilab): Using OpenMP for HEP Framework Algorithm Scheduling. http://cds.cern.ch/record/2712271
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
    void spawnAndWait(
      const std::vector< Task* >&  tasks
    );


    namespace native {
      /**
       * Spawn a ready task into the runtime system of choice.
       */
      void spawnTask(Task* task);


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
       * @param tasks Set of tasks. Is guaranteed to be non-empty.
       */
      void spawnAndWaitAsTaskLoop(
        const std::vector< tarch::multicore::Task* >&  tasks
      );


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
      void processFusedTask(Task*  firstTask, const std::list< tarch::multicore::Task* >& otherTasks, int device);

      /**
       * Drain all task queues
       */
      void drainTaskQueues();


      /**
       * These are the non-blocking (ready) tasks submitted via spawnTask()
       */
      int getNumberOfPendingTasks();
    }


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
      int  mapPendingTasksOntoNativeTasks(int maxTasks);

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
      bool fusePendingTasks(const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction);

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
    }
  }
}


bool operator<( const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs );



