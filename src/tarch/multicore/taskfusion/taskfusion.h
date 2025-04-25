// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <unordered_set>
#include <list>

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/timing/GlidingAverageMeasurement.h"


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
     *
     * @see Consult the page @ref documentation_tarch_task_fusion_strategies
     *   for recommendations and hints on task fusion strategies.
     */
    namespace taskfusion {
      /**
       * Task queue of tasks which we hold back
       *
       * @see tarch::multicore::strategy::Strategy::updateFuseStatistics()
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
        tarch::multicore::BooleanSemaphore         readyTasksSemaphore;
        tarch::multicore::BooleanSemaphore         completedTasksSemaphore;
        tarch::multicore::BooleanSemaphore         statisticsSemaphore;
        int                                        type;
        tarch::timing::GlidingAverageMeasurement   numberOfPendingReadyTasksPerProcessTaskExecution;
        tarch::timing::GlidingAverageMeasurement   probabilityThatTaskIsCompleted;
        int                                        lastProcessReadyTask;

        ReadyFusableTasks(int type_);
      };

      /**
       * Translate a single task into a sequence of (fusable tasks)
       *
       * This routine makes a decision if to spawn a task as native task
       * or to treat it as a fused task. The routine therefore contains all
       * the fusion logic. If we fuse, it first ensure that there's a queue for this task type, and then
       * creates the two helper tasks that will actually handle the queue
       * insertion and the task progression.
       *
       * ## Algorithmic steps
       *
       * The code first of all locks all the tasks through pendingTasksSemaphore
       * and then ensures that there is a task queue for this particular
       * task type. After that, the pointer taskQueue is guaranteed to point
       * to the correct queue.
       *
       * ### Tasks without task numbers (out-dependencies)
       *
       * We cannot split the whole scheduling into two parts, as we do not not
       * have a dependency number. So we insert the tasks straightforward into
       * the out queue. After that, we issue a postprocessing task to handle
       * the fused tasks. This will have no outgoing dependencies either.
       *
       * ### Tasks without in-dependencies
       *
       * We know that we have out-dependencies (see previous case distinction),
       * so we can directly insert the task into the queue and then spawn a
       * ProcessReadyTask which handles this tasks and models the
       * out-dependency.
       *
       * ### Tasks with in-dependencies
       *
       * If the new fusable task has in-dependencies, we have to create a new
       * instance of LogReadyTask and spawn it. We then immediately create
       * a ProcessReadyTask instance with a dependency.
       *
       *
       * ## Decision logic
       *
       * If the max number of tasks that should be fused is smaller one, the
       * user has, by definition switched off all fusion. We therefore
       * immediately spawn the task as a native one.
       *
       *
       * ## Partial serialisation
       *
       * The whole idea of the fusion is that multiple tasks are bundled into
       * one. If there are too many instances of ProcessReadyTask, then they
       * will all grab from the fusion queue and there will never be enough
       * tasks. So it makes sense to serialise the ProcessReadyTask instances
       * if there are less than two times the max tasks in the queue.
       */
      void translateFusableTaskIntoTaskSequence(
        Task*                  task,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber
      );

      void processAllReadyTasks();
    } // namespace taskfusion
  }
}
