// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "multicore.h"


namespace tarch {
  namespace multicore {
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
       * @see The overview page @ref page_tarch_multicore discusses different
       *        task types and how they are mapped onto realisations.
       *
       * @param taskType Unique task (type) number for this task.
       *        Pass in DontFuse if you don't want this task to be fused.
       * @param priority Integer value that indicates what priority you want to
       *        assign a task. Value has to be non-negative. Use
       *        DefaultPriority if there is no bespoke priority.
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

      /**
       * Run the task
       */
      virtual void run() = 0;

      /**
       * Fuse multiple tasks
       *
       * Fuse the task with a list of further tasks. The routine is guaranteed to
       * be called only for tasks with the same taskType. So if you carefully
       * distinguish your tasks, you can downcast all the arguments, as you might
       * know the real type.
       *
       * This operation is invoked on a task. However, it is also given N=otherTasks.size
       * further tasks of the same type. In the routine, you have to process
       * the N+1 tasks in one rush, i.e. all tasks from
       * otherTasks and the task represented by the actual object on which
       * fuse is called. In this case, you return false, as the task on which
       * fuse has been called can be destroyed by the runtime.
       *
       * <h2> Default implementation </h2>
       *
       * My default implementation executes all the passed tasks one by one.
       *
       * <h2> Memory ownership </h2>
       *
       * fuse() has to complete all the instances within otherTasks, but it
       * does not have to delete anything. This is done outside, i.e. the
       * calling routine will delete otherTasks' instances as well as the one
       * it has called fuse() for.
       *
       * @param otherTasks List of tasks to fuse and process. Will all have the
       *         same type as the present object. It is the tasks responsibility
       *         to get these tasks done.
       * @param targetDevice On which device should the task be processed? A
       *         negative number means local host anything greater or equal to
       *         zero denotes an accelerator.
       */
      virtual void fuse(const std::list<Task*>& otherTasks, int targetDevice = Host);

      virtual std::string toString() const;
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
     *
     * Peano's tasking API is a plain class-based implementation of a task
     * system. Many modern APIs such as oneTBB favour a functor-based API.
     * The latter approach add nothing new, as C++ internally breaks down
     * functors into classes with an operator(). In Peano, we mirror this
     * behaviour, i.e. start with a class design and then offer this class
     * on top which allows you to pipe in a functor instead of implementing
     * your run() within a subclass.
     *
     * Most people using lambdas for this class will define a lambda within
     * a function which catches all relevant data and then pass this lambda
     * into a task. This implies that this lambda object will be destroyed
     * once the spawning routine terminates, even though the task might not
     * have been executed at this point. Therefore, this routine copies the
     * lambda.
     *
     * The functor is a 1:1 translation of Task's run(). It takes no arguments,
     * and it returns a bool which indicates if this task has to rerun.
     * Returning false signals that this task is done and can be discarded.
     * Returning true signals that the task is done, but the same task has to
     * be re-executed. It is up to the tasking runtime to decide if it will
     * re-execute immediately again or at a later point.
     *
     * A typical usage looks similar to
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *   tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor (
     *     tarch::multicore::Task::DontFuse,
     *     tarch::multicore::Task::DefaultPriority,
     *     [=,this]()->bool {
     *        ...
     *        return false;
     *     }
     *   );
     *
     *   tarch::multicore::spawnTask( 
     *     newTask, 
     *     tarch::multicore::NoInDependencies, 
     *     myNumber 
     *   );
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *
     */
    class TaskWithCopyOfFunctor: public Task {
    private:
      /**
       * See the outer class description for an explanation why this is an
       * attribute, i.e. why we copy the functor here always.
       */
      std::function<void()> _taskFunctor;

      #if PeanoDebug>0
      std::string           _taskDescription;
      #endif

    public:
      TaskWithCopyOfFunctor()                             = delete;
      TaskWithCopyOfFunctor(const TaskWithCopyOfFunctor&) = delete;

      /**
       * @param taskType See superclass documentation
       * @param priority See superclass documentation
       * @param taskFunctor Functor that's then internally copied. Functor
       *   returns indicator if task has to rerun. See class documentation.
       */
      TaskWithCopyOfFunctor(
        int taskType,
        int priority,
        const std::function<bool()>& taskFunctor
        #if PeanoDebug>0
        , const std::string&           taskDescription = ""
        #endif
      );

      virtual void run() override;

      virtual std::string toString() const override;
    };

    /**
     * An empty task
     *
     * Empty tasks are useful in many cases to create synchronisation points in
     * task graphs.
     */
    class EmptyTask: public Task {
      public:
        EmptyTask() = delete;

        /**
         * Create empty task
         *
         * Every empty task is of the type DontFuse.
         */
        EmptyTask(int priority);

        virtual void run() override;

        virtual std::string toString() const override;
    };

    /**
     * Frequently used implementation for job with a functor.
     *
     * Cousin implementation to TaskWithCopyOfFunctor which does not copy the
     * underlying functor but holds a reference to it. This implies that you
     * have to ensure that the functor remains valid after you have spawned
     * the task.
     *
     * It can pay off to use this variant for very expensive functors, i.e.
     * functors that internally copy a lot of things. In most cases, you won't
     * need this class. I recommend to start with the copy version always.
     *
     * Please consult TaskWithCopyOfFunctor for further information on the
     * used attributes.
     */
    class TaskWithoutCopyOfFunctor: public Task {
    private:
      /**
       * See the outer class description for an explanation why this is an
       * attribute, i.e. why we copy the functor here always.
       */
      std::function<void()>& _taskFunctor;

    public:
      TaskWithoutCopyOfFunctor()                                = delete;
      TaskWithoutCopyOfFunctor(const TaskWithoutCopyOfFunctor&) = delete;

      TaskWithoutCopyOfFunctor(int taskType, int priority, std::function<void()>& taskFunctor);

      virtual void run() override;
    };
  }   // namespace multicore
} // namespace tarch

bool operator<(const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs);
