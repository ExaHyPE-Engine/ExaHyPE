// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <set>

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace tarch {
  namespace multicore {
    /**
     * Forward declaration
     */
    class Task;

    /**
     * @namespace tarch::multicore::taskfusion Task fusion
     *
     * Task fusion is now relatively easy and all mapped onto plain old tasks:
     * Whenever we get a task (in the example below we get three yellow tasks)
     * with dependencies, we split each task up into two tasks each:
     *
     * @image html multicore_task_flow-00.png
     *
     * One task is a preparation task, and the other one is the actual
     * processing. The prep task has all the complex incoming dependencies. The
     * processing task is part of a straight task sequence. Both the prep task
     * and the processing task know the actual task, which is not part of the
     * actual task graph anymore. It is associated with these two wrapper
     * classes.
     *
     * @image html multicore_task_flow-01.png
     *
     * If the prep task becomes active, we do not evaluate the task. Instead,
     * we dump the task into the queue of ready tasks.
     *
     * @image html multicore_task_flow-02.png
     * @image html multicore_task_flow-03.png
     *
     */
    namespace taskfusion {
      /**
       * Task queue of tasks which we hold back
       */
      struct ReadyFusableTasks {
        ~ReadyFusableTasks()                       = default;
        ReadyFusableTasks(const ReadyFusableTasks&) = delete;

        using Tasks = std::list<tarch::multicore::Task*>;
        Tasks                                      tasks;
        tarch::multicore::BooleanSemaphore         semaphore;
        int                                        type;

        ReadyFusableTasks(int type_):
          type(type_) {}
      };

      /**
       * Translate a single task into a sequence of (fusable tasks)
       *
       * First ensure that there's a queue for this task type, and then
       * create the two helper tasks that will actually handle the queue
       * insertion and the task progression.
       *
       * @return New task that is to be submitted instead.
       */
      void translateFusableTaskIntoTaskSequence(
        Task*                        task,
        const std::set<TaskNumber>&  inDependencies,
        const TaskNumber&            taskNumber
      );
    } // namespace internal
  }
}
