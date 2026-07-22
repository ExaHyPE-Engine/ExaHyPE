// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/multicore.h"

namespace tarch {
  namespace multicore {
    /**
     * Abstract superclass for a task.
     */
    class Task {
    protected:
      const int _taskType;
      int       _priority;

    public:
      static constexpr int NoTaskType      = -1;
      static constexpr int DefaultPriority = 1024;

      /**
       * Construct task
       *
       * @param taskType Unique task (type) number for this task.
       * @param priority Integer value that indicates which priority to assign
       *        to a task. Value has to be non-negative. Use DefaultPriority
       *        if there is no bespoke priority.
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
       * @return This task has to be executed again. In most cases, you should
       *   return false, to indicate that this task has finished.
       */
      virtual bool run() = 0;

      virtual std::string toString() const;
    };

    /**
     * Helper class to administer tasks within a queue
     *
     * It works with task objects as well as with pointers to tasks.
     */
    class TaskComparison {
    public:
      bool operator()(const Task& lhs, const Task& rhs) const;
      bool operator()(Task* lhs, Task* rhs) const;
    };

    /**
     * Frequently used implementation for a task with a functor.
     *
     * Peano's tasking API is a plain class-based implementation of a task
     * system. Many modern APIs such as oneTBB favour a functor-based API.
     * The latter approach adds nothing new, as C++ internally breaks down
     * functors into classes with an operator(). Peano mirrors this behaviour,
     * i.e. starts with a class design and then offers this class on top,
     * which allows you to pipe in a functor instead of implementing your
     * run() within a subclass.
     *
     * A lambda for this class is typically defined inside a function, where
     * it captures all relevant data, and is then passed into a task. The
     * lambda object is thus destroyed once the spawning routine terminates,
     * even though the task may not have run at that point. This class
     * therefore copies the functor.
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
     *   tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor(
     *     tarch::multicore::Task::NoTaskType,
     *     tarch::multicore::Task::DefaultPriority,
     *     [=, this]() -> bool {
     *        ...
     *        return false;
     *     }
     *   );
     *
     *   tarch::multicore::spawnTask(newTask);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    class TaskWithCopyOfFunctor : public Task {
    private:
      /**
       * Held by value, as the functor handed in usually goes out of scope
       * before the task runs. See the class description.
       */
      std::function<bool()> _taskFunctor;

#if PEANO_DEBUG
      std::string _taskDescription;
#endif

    public:
      TaskWithCopyOfFunctor()                             = delete;
      TaskWithCopyOfFunctor(const TaskWithCopyOfFunctor&) = delete;

      /**
       * @param taskType See superclass documentation
       * @param priority See superclass documentation
       * @param taskFunctor Functor which is copied internally. It returns an
       *   indicator whether the task has to rerun. See class documentation.
       */
      TaskWithCopyOfFunctor(
        int                          taskType,
        int                          priority,
        const std::function<bool()>& taskFunctor
#if PEANO_DEBUG
        ,
        const std::string& taskDescription = ""
#endif
      );

      bool run() override;

      std::string toString() const override;
    };

    /**
     * An empty task
     *
     * Empty tasks are useful in many cases to create synchronisation points in
     * task graphs.
     */
    class EmptyTask : public Task {
    public:
      EmptyTask() = delete;

      /**
       * Create empty task
       *
       * Every empty task is of the type NoTaskType.
       */
      EmptyTask(int priority);

      bool run() override;

      std::string toString() const override;
    };

    /**
     * Frequently used implementation for a task with a functor.
     *
     * Cousin implementation to TaskWithCopyOfFunctor which does not copy the
     * underlying functor but holds a reference to it. The caller therefore
     * has to keep the functor valid after the task has been spawned.
     *
     * This variant suits very expensive functors, i.e. functors which
     * internally copy a lot of data. In most cases TaskWithCopyOfFunctor is
     * the better starting point.
     *
     * Please consult TaskWithCopyOfFunctor for further information on the
     * used attributes.
     */
    class TaskWithoutCopyOfFunctor : public Task {
    private:
      /**
       * Held by reference, so the functor has to outlive the task. See the
       * class description.
       */
      std::function<bool()>& _taskFunctor;

    public:
      TaskWithoutCopyOfFunctor()                                = delete;
      TaskWithoutCopyOfFunctor(const TaskWithoutCopyOfFunctor&) = delete;

      TaskWithoutCopyOfFunctor(int taskType, int priority, std::function<bool()>& taskFunctor);

      bool run() override;
    };
  } // namespace multicore
} // namespace tarch

bool operator<(const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs);
