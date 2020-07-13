// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_TASKS_H_
#define _TARCH_MULTICORE_TASKS_H_


#include <functional>
#include <vector>


namespace tarch {
  namespace multicore {

    /**
     * Tells task/thread to yield, i.e. to allow other tasks/threads to run.
     * Typically to be used within busy-waiting/polling loops.
     */
    void yield();

    /**
     * Abstract super class for a job.
     */
    class Task {
      protected:
        int   _priority;
      public:
        Task( int priority=0 );

        int getPriority() const;
        void setPriority( int priority );

        virtual bool run() = 0;

        /**
         * This operation is called prior to run(). We try to make it as
         * close to run as possible. The idea is that codes use it to
         * insert their prefetch macros. Peano gives no guarantee that this
         * operation is called, i.e. everything done here is solely
         * optional.
         *
         * Prefetching for Intel is specified at
         *
         * https://software.intel.com/en-us/node/684213
         *
         * or
         *
         * https://software.intel.com/en-us/cpp-compiler-developer-guide-and-reference-cacheability-support-intrinsics-1
         *
         * There are basically the following modes: _MM_HINT_T0,
         * _MM_HINT_T1, _MM_HINT_T2 and _MM_HINT_NTA. T0 means a
         * prefetch into all cache levels. T1 means into all levels
         * except L1 (L1 might be populated but we do not enforce it),
         * T2 means at least L3. NTA is the most defensive one.
         * It loads stuff as close to the core as possible without
         * polluting any other caches.
         *
         * We usually use mode/hint _MM_HINT_NTA for data on machines with
         * Optane. This makes the data be loaded into L3. For urgent
         * computations it might however make sense to use T0.
         *
         */
        virtual void prefetch();
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
        TaskWithCopyOfFunctor( const std::function<bool()>& taskFunctor );

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
        TaskWithoutCopyOfFunctor( std::function<bool()>& taskFunctor );

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
  }
}

#endif

