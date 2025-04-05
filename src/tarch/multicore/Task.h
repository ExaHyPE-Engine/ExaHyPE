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
       * ## Task types
       *
       * Each task in Peano has to have a task type. All tasks that do the same,
       * i.e. are of the same type, should have the same task type integer
       * marker. Peano's tarch can use multiple tasks of the same type and
       * fuse/batch them into one task call. However, if you don't want Peano
       * to even thing about task fusion, pass in DontFuse as argument for the
       * type. In this case, the tasking subsystem becomes agnostic of the
       * type.
       *
       * In principle, it is totally up to the user to manage the task type
       * numbers. However, they have to be unique. This quickly becomes
       * challenging - notably if you combine multiple solvers. Therefore,
       * Peano offers a factory mechanism peano4::parallel::getTaskType()
       * to create task types, which Peano applications should use.
       *
       * If you have a proper task class, I recommend that you introduce a
       * static field for this class and you initialise it with the outcome
       * of peano4::parallel::getTaskType(). If there's one dedicated code
       * line in your code which produces exclusively all the tasks of one
       * type, you can also do
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * static int taskType = peano4::parallel::getTaskType("MyFancyClassName");
       *
       * ...
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       *
       * ## Task numbers
       *
       * Some codes also have task numbers. They need this, if they have to
       * identify unique tasks among a set of tasks of the same type.
       * Applications for that are task that are offloaded to other ranks. For
       * the latter tasks, we have to match data sent back by another rank to
       * the task that would have produced these results locally. Anyway, task
       * numbers are not baked into the generic interface, as we don't need
       * them for all tasks all the time, and I want to avoid that the
       * construction of unique task numbers becomes too expensive.
       *
       * Some extensions that work with task numbers provide factory methods
       * for these, which is kind of good practice. ExaHyPE for example
       * offers exahype2::EnclaveTask::reserveTaskNumber() which ensures that
       * task numbers are unique among multiple threads and also recycled,
       * such that they do not overflow.
       *
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
       * Fuse the task with a list of further tasks. The routine is guaranteed
       * to be called only for tasks with the same taskType. As long as you
       * take care that every task type has a unique number, you can downcast
       * all the objects within otherTasks into the type this virtual function
       * is called for.
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
       * Create new task from a functor
       *
       * This is the standard version of a task that should be used with a
       * functor. Obviously, you can use the version without the copying,
       * but if and only if you can ensure that the functor remains valid even
       * once you leave the surrounding scope. This version copies the
       * functor and hence is fine even if the functor had been created (via
       * auto) directly before we construct the task. However, we implicitly
       * assume that the functor's variables all remain available, too, i.e.
       * we'd expect in most cases that the functor catches all surrounding
       * variables through copying.
       *
       * ## Tasks capturing external data
       *
       * If tasks capture and alter external data, constructing them through a
       * functor that's copied doesn't work, as C++ implicitly assumes that
       * this functor has to be const. Therefore, you manually have to set
       * the functor to mutuable. Here is an example from the multigrid
       * extension:
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *     auto invertMatrixFunctor = [inverseOperator, marker, OperatorSize]() mutable -> void {
       *      std::memcpy(
       *         ...
       *      );
       *     };
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       *
       * ## Fusion
       *
       * You can have functor tasks with generic  fusion. For this, simply
       * pass in a proper unique task type, so we can distinguish tasks from
       * each other. This is best done through a factor method as explained
       * in the superclass.
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * static int taskType = peano4::parallel::getTaskType("MyFancyClassName");
       *
       * tarch::multicore::TaskWithCopyOfFunctor* updateResidualTask = new tarch::multicore::TaskWithCopyOfFunctor(
       *   taskType,
       *   tarch::multicore::Task::DefaultPriority,
       *   [=](...) -> void {
       *     ...
       *   }
       * );
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       *
       * @param taskType See superclass documentation
       * @param priority See superclass documentation
       * @param taskFunctor Functor that's then internally copied. Functor
       *   returns indicator if task has to rerun. See class documentation.
       */
      TaskWithCopyOfFunctor(
        int taskType,
        int priority,
        const std::function<void()>& taskFunctor
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
