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
  /**
   * Our tasking layer wraps around various tasking backends. The most popular backend is
   * OpenMP, but you can also use Intel's OneTBB or C++ tasking. Per thread in the system,
   * we have one task queue within Peano. Furthermore, we have a global task queue. The
   * global system's behaviour is controlled via an orchestration object which tells
   * Peano's tasking how to handle individual tasks.
   *
   * ## Orchestration flavours
   *
   * If you set a BSP-type task orchestration, the code will spawn one task per subdomain
   * on a rank and immediately process any enclave tasks, i.e. individual tasks, when they
   * drop into the tasking subsystem. In this case, Peano's own task queues will be empty
   * all the time. They are not used.
   *
   * There is a native orchestration type which immediately spawns all tasks into the
   * underlying tasking system. Again, Peano's internal task queues are not used.
   *
   * All other orchestration flavours dump their individual tasks into the thread-local
   * queue first. This can be done without any semaphore or similar as there's one task
   * queue per thread. Once a local BSP section has finished completely, the thread-local
   * task queues are copied over into the global task queue. It again depends on the
   * orchestration strategy how we proceed with this global task queue from hereon.
   *
   * While a BSP section is waiting it can poll its thread-local task queues (again without
   * any additional lock) and process tasks from there, or it can poll the global task
   * queue which might have been befilled by someone else.
   *
   * Whenever we think we could fuse tasks into one large task assembly, we first commit
   * the thread-local task queue into the global task queue and then we try to combine
   * the tasks there.
   *
   * The three-layer approach with a thread-local queue, a global queue and the actual
   * tasking subsystem is necessary. We need Peano's specific queues to be able to merge
   * tasks on-the-fly. We need the thread-local queues to be fast. With only one global
   * queue, we found that a lot of codes basically compete for the locks to spanwn tasks
   * into this queue.
   *
   *
   * ## Statistics
   *
   * If the Peano statistics are enabled, the tasking backend will sample some quantities:
   *
   * - "tarch::multicore::bsp-concurrency-level" Typically corresponds to the
   *    number of fork-join traversal tasks.
   * - "tarch::multicore::global-pending-tasks" Global pending tasks.
   * - "tarch::multicore::thread-local-pending-tasks" Pending tasks per thread
   *   which are not yet committed to the global queue.
   * - "tarch::multicore::fuse-tasks" Number of tasks which have been fused.
   *
   * Depending on the chosen backend, you might get additional counters on top.
   *
   *
   *
   * @see tarch::multicore::orchestration::StrategyFactory
   */
  namespace multicore {
    void setOrchestration( tarch::multicore::orchestration::Strategy* realisation );

    /**
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
        bool canFuse() const;

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
        TaskWithoutCopyOfFunctor( int taskType, int priority, std::function<bool()>& taskFunctor );

        virtual bool run() override;
    };


    /**
     * @param threadCount -1 if you are interested in global number of tasks or a
     *   non-negative integer if you are only interested in one particular thread
     *   and its specific tasks.
     */
    int getNumberOfPendingTasks(int threadCount);


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
     * <h2> Behaviour </h2>
     *
     * There are two different types of behaviour, depending on your runtime's state:
     * If the taskProgressionStrategy is equal to TaskProgressionStrategy::MapOntoNativeTask,
     * the thin layer forwards the task straightaway to the language-specific back-end. The
     * task thus is mapped 1:1 to OpenMP, e.g. Otherwise, the routine does take the task and
     * enqueues is the local queue nonblockingTasks.
     *
     *
     * <h2> SmartMPI </h2>
     *
     * If you compile with SmartMPI, this is the one and only point where the control flow
     * diverges: If a task is smart, the routine takes the task and forwards it to SmartMPI.
     *
     *
     * @see tarch::multicore::spawnAndWait()
     * @see processPendingTasks(int)
     *
     * @param someoneWillWaitForThisTaskNumber Memorise future. A task that asks to be
     *   re-scheduled always has to pass false.
     */
    void spawnTask(Task*  job);

    /**
     * - Tell the orchestration that a BSP section starts
     * - Invoke the native implementation (and tell it what orchestration
     *   strategy is to be used)
     * - Tell the orchestration that the BSP section has terminated
     * - If there are task pending and the orchestration restricts the
     *   number of tasks to hold back, map them onto native tasks.
     */
    void spawnAndWait(
      const std::vector< Task* >&  tasks
    );

    namespace native {
      void spawnTask(Task* task);
      void spawnAndWait(
        const std::vector< Task* >&  tasks,
        orchestration::Strategy*     strategy
      );
      /**
       * Process a fused task
       *
       * A fused task is a task (which can be fused) and a list of further tasks which
       * are of the same type and, hence, can be fused, too. The list can be empty.
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
      int getNumberOfPendingTasks();

      /**
       * This routine is used by processPendingTasks() whenever there are no other tasks
       * left anymore.
       */
      void deferRunningThreadAndContinueWithOtherTasks();
    }


    namespace internal {
      /**
       * Progression routine within spawnAndWaitAsExplicitTasksWithPolling()
       *
       * This function has to be called within spawnAndWaitAsExplicitTasksWithPolling()
       * whenever a thread has finished its local partition and should now
       * process further tasks. The polling consists of two ingredients:
       *
       * 1. A guard loop ensures that we don't poll too long or too often.
       * 2. The actual loop body ensures that we progress tasks while we
       *    wait for the outer task group to finish, i.e. for sibling tasks
       *    to terminate.
       *
       *
       * ## Guard loop
       *
       * The guard loop consists of three checks that all have to hold:
       *
       * - If the number of pending (BSP) tasks equals zero, we terminate
       *   immediately. That is, we only continue to progress while another
       *   thread traverses a subdomain and hence might produce new tasks.
       * - If the number of pending tasks is bigger or equal to the number
       *   of physical threads, we terminate immediately. This can happen
       *   if we have more subdomains that have to be mapped onto tasks than
       *   we have actual threads. So better terminate this task immediately
       *   to give the runtime the opportunity to process real compute tasks.
       * - If not all tasks have been spawned yet, we also terminate
       *   immediately. If the runtime hasn't had the opportunity yet to
       *   kick off the other tasks, we better ensure that happens asap.
       *
       * I originally wanted to integrate this guard loop into the routine.
       * However, that does not work. The task loop is defined over some
       * atomics, and the type of such atomics depends on the chosen
       * environment. They have a different type for OpenMP vs TBB, e.g. So
       * the user has to wrap the function call into a code snippet similar
       * to
       *
       *          bool continueToProcessPendingTasks =
       *            pendingTasks>0
       *            and
       *            pendingTasks<tarch::multicore::Core::getInstance().getNumberOfThreads()
       *            and
       *            spawnedTasks==NumberOfThreads;
       *          while ( continueToProcessPendingTasks ) {
       *            continueToProcessPendingTasks = ::tarch::multicore::internal::progressTasksWithinSpawnAndWaitAsExplicitTasksWithPolling(
       *             strategy
       *            );
       *          }
       *
       * @todo Not sure if we need that additional loop here. It came from
       *   a misconception
       *
       * This snippet highlights a fourth consideration: If we have tried to
       * process a pending task but failed, then we terminate as well. This is
       * not necessary for all solvers, but
       *
       * - if other compute kernels/tasks fork further, an active polling keeps
       *   the current thread busy, i.e. strips those other kernels off the
       *   opportunity to grab threads in return. So active polling severely
       *   slows down the calculation. In the worst case, it leads to deadlocks.
       * - if there is an extreme ill-balance, a lot of threads will end up
       *   competing for the global lock. In return, the task still computing
       *   will struggle to enqueue new tasks rapidly. Once again, aggressive
       *   polling would slow down the simulation even though it should, in
       *   theory, be faster.
       *
       *
       * ## Guard loop
       *
       * We run through a cascade of checks:
       *
       * 1. If we have to fuse tasks and hold some tasks locally, copy my own
       *    tasks over into the global queue and invoke tarch::multicore::internal::fusePendingTasks().
       *    The latter one operates on the global queue. We move our tasks over
       *    first to ensure that there are as many tasks as possible in the
       *    global queue for the fusion (everybody copies over). No matter
       *    weather we have then successfully fused or not - we return a true
       *    as we have actually done something meaningful.
       * 2. If we do not fuse tasks but hold some tasks locally, process one of
       *    my own tasks and return. The idea here is that a thread might
       *    create a lot of tasks and then later find out that another
       *    thread still runs. In this case, it is reasonable that we try to
       *    process our own tasks first.
       * 3. If we have no local tasks left, should fuse, and the global queue
       *    is not empty, try a fuse. If that's not successful, process half of
       *    the global tasks that hang out there.
       * 4. If we have no local tasks left, should not fuse, and the global
       *    queue is not empty, process half of the global tasks there.
       * 5. If we have no local tasks left, and if the global queue is also
       *    empty, we ask all other threads to copy their local tasks into the
       *    global queue. If that's successful, i.e. if we have moved some
       *    tasks, we return true. We could now immediately process tasks, but
       *    then we would have to replicate the logic of 3 and 4. So we better
       *    return immediately and then rely on the fact that this routine will
       *    be called again.
       *
       * @return Something meaningful has been done
       */
      bool progressTasksWithinSpawnAndWaitAsExplicitTasksWithPolling(
        tarch::multicore::orchestration::Strategy*     strategy
      );

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
       * Set up the internal queues that our wrapper around the tasking back-end
       * requires. If we worked with only one task queue, this would not be
       * necessary, but we found such a solution deterimental. Instead, it is
       * way better to work with one queue per thread, so we can spawn enclave
       * tasks without any additional locks.
       */
      void configureInternalTaskQueues(int numberOfTasks);

      /**
       * Commit local task queues into global queue
       *
       * If we work with thread-local queues, we have to flush them over into the
       * global queue eventually. To do so, this routine is to be called either
       * per thread or with a -1 to roll all local queues over into the one global
       * task queue.
       *
       * ## Deadlocks
       *
       * We found that the commitment to the global queue is prone to starvation
       * effects: If we have 127 threads hammering the global queue for work and
       * if there's one thread left over, the one thread will struggle to get its
       * message into the global queue. This is particularly cumbersome, if the
       * local thread doesn't have any message:
       *
       * My original implementation locked the global queue and then copied the
       * local tasks over. If the local tasks are empty, the copy is becoming a
       * trivial operation. In the situation sketched above, it however is
       * problematic, as I still would request a global lock, even though nothing
       * will be copied over. Therefore, we found it advantageous to add a separate
       * case distinction: If we don't have a local task, we should not even lock
       * the queue (and, obviously, don't have to copy over either).
       *
       * This insight materialises in
       *
       *           else if ( not localNonblockingTasks[threadNumber].empty() ) {
       *
       *
       * ## Flush all queues
       *
       * In line with the observation above (and in terms of code redundancy), we
       * might make the flush a loop over all queues and then call the individual
       * flush per queue. This would mean that we potentially lock the global
       * queue p times for p threads.
       *
       * I usually flush all queues if and only if I'm at the end of a BSP section.
       * So I avoid repeated locks, lock once and then flush all of the task queues
       * in one go before I release the lock.
       *
       *
       * @param threadNumber -1 if you want to flush all thread buffers
       */
      void copyInternalTaskQueuesOverIntoGlobalQueue(int threadNumber);

      /**
       * Take one task that is tied to the thread-local queue, i.e. not yet committed
       * to the global queue, and process it. As we assume that we work thread-local,
       * there's no need for a global semaphore when we access the queue.
       */
      bool processThreadLocalTask(int threadNumber);
    }
  }
}


bool operator<( const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs );



