// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_TASKS_H_
#define _TARCH_MULTICORE_TASKS_H_


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
     * Tells task/thread to yield, i.e. to allow other tasks/threads to run.
     * Typically to be used within busy-waiting/polling loops.
     *
     * yield() has to be called very very carefully on the user side. If you wait
     * for task outcomes from low priority tasks, e.g., do not call yield
     * immediately. Call processPendingTasks(). It is only if this routine tells
     * you that no background tasks are available anymore that you should yield.
     */
    void yield();

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
        const int   _id;
        const int   _taskType;
        int         _priority;
      public:
        static constexpr int DefaultPriority = 0;

        /**
         * @param id       Unique number of the task
         * @param taskType Unique task (type) number for this task. All tasks that do 
         *        the same, i.e. are of the same type, should have the same task type
         *        integer marker. However, they all should have different ids.
         */
        Task( int id, int taskType, int priority );

        virtual ~Task() {}

        int getTaskType() const;
        int getTaskId() const;
        int getPriority() const;
        void setPriority( int priority );

        virtual bool run() = 0;

        /**
         * Fuse the task with a list of further tasks. The routine is guaranteed to 
         * be called only for tasks with the same taskType. So if you carefully 
         * distinguish your tasks, you can downcast all the arguments, as you might
         * know the real type.
         *
         * <h2> Default implementation </h2>
         * 
         * My default implementation executes all the passed tasks and then returns 
         * the original task back, i.e. this one is not executed.
         *
         * <h2> Memory ownership </h2>
         * 
         * The ownership for otherTasks goes over to fuse, i.e. you don't have to 
         * delete anything anymore.
         *
         * @return Is the present task still to be executed or can the runtime 
         *         destroy it straightaway?
         */
        virtual bool fuse( const std::list<Task*>& otherTasks );

        /**
         * Is off by default.
         */
        virtual bool canFuse() const;

        /**
         * Follows the signature of fuse(). Please hand in an (empty) list
         * of producedTasks which is befilled by split(). The routine returns
         * true if the original task that is split still has to be executed.
         * It returns false if you can safely delete the task.
         *
         * We require a strict parent-child relation on the splitting, i.e.
         *
         * todo: Alles nicht ausgegoren!
         *
         * @return Is the present task still to be executed or can the runtime
         *         destroy it straightaway?
         */
        virtual bool split(std::list<Task*>& producedTasks);
        virtual bool canSplit() const;

        /**
         * If this attributes holds (default is false), then it is safe for
         * the scheduler to cast the task object into a SmartMPI task.
         */
        virtual bool isSmartMPITask() const;
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
        TaskWithCopyOfFunctor( int id, int taskType, int priority, const std::function<bool()>& taskFunctor );

        bool run() override;
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
        TaskWithoutCopyOfFunctor( int id, int taskType, int priority, std::function<bool()>& taskFunctor );

        bool run() override;
    };

    int getNumberOfPendingTasks();

    /**
     * Process a few tasks from my backlog of tasks
     *
     * This routine tries to complete maxTasks. The behaviour of the routine
     * depends on the value of taskProgressionStrategy. We use different
     * strategies at different points in time.
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
     * Process a particular task.
     *
     * @param number  Has to be a number acquired before via reserveTaskNumber.
     * @return Found this one and have done it.
     */
    //bool processTask(int number);

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
      void spawnTask(Task*  job);
      void spawnAndWait(
        const std::vector< Task* >&  tasks,
        int                          numberOfTasksToHoldBack
      );
      void yield();
      int getNumberOfPendingTasks();
    }
  }
}


bool operator<( const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs );


#endif

