// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_TASKS_H_
#define _TARCH_MULTICORE_TASKS_H_


#include <functional>
#include <vector>
#include <list>
#include <string>


namespace tarch {
  namespace multicore {
    enum class Realisation {
      MapOntoNativeTasks,
      HoldTasksBackInLocalQueue,
      HoldTasksBackInLocalQueueAndBackfill,
      HoldTasksBackInLocalQueueBackfillAndRelease,
      HoldTasksBackInLocalQueueMergeBackfillAndRelease
    };

    std::string toString( Realisation realisation );

    /**
     * Use toString() to see valid options
     */
    void parseRealisation( const std::string& realisation );
    std::string getListOfRealisations();
    void setRealisation( Realisation realisation );
    Realisation getRealisation();

    /**
     * Constrain the fusion.
     */
    void configureTaskFusion( int maxNumberOfFusedAssemblies, int maxSizeOfFusedTaskSet );

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
         *        the same, i.e. are of hte same type, should have the same task type 
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
     * @param maxTasks You may pass 0 if you want to tell the task system that you
     *   don't want to use your main thread to process any tasks but you would like
     *   the task system to occupy some other threads with task processing. This is
     *   a routine we typically invoke just before we enter a (quasi-)serial program
     *   phase such as the data exchange.
     *
     * @return There have been tasks
     */
    bool processPendingTasks(int maxTasks = getNumberOfPendingTasks());

    /**
     * Process a particular task.
     *
     * @param number  Has to be a number acquired before via reserveTaskNumber.
     * @return Found this one and have done it.
     */
    bool processTask(int number);

    /**
     * Kick out a new job. The job's type has to be set properly: It
     * has to be clear whether the job is a job or even a task, i.e. a
     * special type of job. See JobType.
     *
     * Ownership goes over to Peano's job namespace, i.e. you don't have
     * to delete the pointer.
     */
    void spawnTask(Task*  job);

    /**
     * The routine deletes all the passed arguments.
     */
    void spawnAndWait(
      const std::vector< Task* >&  tasks
    );

    namespace native {
      void spawnTask(Task*  job);
      void spawnAndWait( const std::vector< Task* >&  tasks );
      void yield();
      int getNumberOfPendingTasks();
    }
  }
}


bool operator<( const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs );


#endif

