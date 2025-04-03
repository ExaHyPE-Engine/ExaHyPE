// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <unordered_set>
#include <list>

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace tarch {
  namespace multicore {
    /**
     * Forward declaration
     */
    class Task;

    /**
     * @namespace tarch::multicore::taskfusion
     *
     * Task fusion means that a set of tasks are grabbed and mapped onto one
     * large physical tasks instead of being processed one by one. This can
     * reduce task administration overheads, but it also allows the routine
     * handling a set of tasks to exploit commonalities or the larger intra-task
     * concurrency.
     *
     *
     * Whenever we get a task (in the example below we get three yellow tasks)
     * with dependencies that can be fused, we split each task up into two
     * tasks each:
     *
     * @image html multicore_task_flow-00.png
     *
     * One task is a preparation task (red), and the other one is the actual
     * processing (green). The prep task has all the complex incoming dependencies. The
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
     * The preparation is done through tarch::multicore::taskfusion::LogReadyTask,
     * which is rather trivial. The actual task execution and fusion is then
     * realised through and instance of ProcessReadyTask which holds also all
     * race condition logic. See the class documentation. This processing task
     * finally also serves as placeholder for outgoing dependencies.
     *
     * To ensure that fusion is only applied to tasks of the same type, we store
     * the fusable tasks in a map indexed by the task type. This map is called
     * pendingTasksThatCanBeFused.
     */
    namespace taskfusion {
      /**
       * Task queue of tasks which we hold back
       */
      struct ReadyFusableTasks {
        ~ReadyFusableTasks()                       = default;
        ReadyFusableTasks(const ReadyFusableTasks&) = delete;

        /**
         * Each task in the queue is identified through a pointer to the
         * task plus a task (dependency) number.
         */
        using Tasks = std::list< std::pair<tarch::multicore::Task*, int> >;
        using FinishedTaskNumbers = std::unordered_set<int>;

        Tasks                                      tasks;
        FinishedTaskNumbers                        completedTaskNumbers;
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
       * ## Algorithmic steps
       *
       * The code first of all locks all the tasks through pendingTasksSemaphore
       * and then ensures that there is a task queue for this particular
       * task type. After that, the pointer taskQueue is guaranteed to point
       * to the correct queue.
       *
       * If the new fusable task has in-dependencies, we have to create a new
       * instance of LogReadyTask and spawn it. If this is not the case, we can
       * insert the task into the respective queue directly.
       *
       * If there are no out-dependencies, we can check if we can process all
       * tasks directly as one fused chunk. If there are not enough tasks there
       * yet, just skip it (see comments on processAllReadyTasks() which kind
       * of flushes these type of tasks). If there are out-dependencies, we
       * have to issue a ProcessReadyTask.
       *
       *
       * ## Optimisation for tasks without in-dependencies
       *
       * If we have no-dependencies, we can directly insert task into the set
       * of pending tasks and hence avoid the overhead associated with a
       * dedicated read task.
       *
       * @return New task that is to be submitted instead.
       */
      void translateFusableTaskIntoTaskSequence(
        Task*                        task,
        const std::set<TaskNumber>&  inDependencies,
        const TaskNumber&            taskNumber
      );

      void processAllReadyTasks();
    } // namespace taskfusion
  }
}
