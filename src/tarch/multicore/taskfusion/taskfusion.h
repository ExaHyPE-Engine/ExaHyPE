// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <map>

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
     * concurrency. Due to the latter argument, task fusion is Peano's favourite
     * technique to keep GPUs busy.
     *
     * There are many ways how task fusion can be implemented. In Peano, the
     * different strategies are controlled through FusionImplementation which
     * in turn is set by the multicore orchestration. More information is
     * provided in the context of @ref documentation_tarch_task_fusion_strategies.
     * Here, we summarise more low-level implementation details.
     *
     * To ensure that fusion is only applied to tasks of the same type, we store
     * the fusable tasks in a map indexed by the task type. The most important
     * type in the context of fusion therefore is FusableTaskQueue.
     *
     * @see Consult the page @ref documentation_tarch_task_fusion_strategies
     *   for recommendations and hints on task fusion strategies.
     */
    namespace taskfusion {
      struct FusableTasksQueue;

      /**
       * Delegates to implementation-specific variants, i.e. to the one
       * selected by the orchestration.
       */
      void translateFusableTaskIntoTaskSequence(
        Task*                  task,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber
      );

      /**
       * Run over all known queues and invoke drainReadyTasks() on each one.
       */
      void processAllReadyTasks();

      /**
       * Realisation for translateFusableTaskIntoTaskSequence() for ProducerConsumerTaskPairs
       *
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
      void translateFusableTaskIntoTaskSequence_ProducerConsumerTaskPairs(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       * Implementation of translateFusableTaskIntoTaskSequence() if we use LazyEvaluation.
       *
       * @see @ref documentation_tarch_task_fusion_strategies for some context for this routine.
       */
      void translateFusableTaskIntoTaskSequence_LazyEvaluation(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      void translateFusableTaskIntoTaskSequence_EagerConsumerTasks(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       * Delegates to implementation-specific variants.
       */
      void ensureHeldBackTasksAreMappedOntoNativeTasks(const std::set<TaskNumber>& inDependencies);

      /**
       * Implementation of waitForTasks() if we use LazyEvaluation.
       *
       * We don't know, at this point, what the type of each in dependency is.
       * Therefore, we simply loop over all queues, i.e. all task types that we
       * are aware of.
       *
       * First, we look in each queue if it hosts the right task. If not, we
       * try to invoke the fusion. If that fails, i.e. the task is not completed
       * yet, we issue another loop over all task types, i.e. queues, and process
       * one task at a time. This is a while loop, i.e. by the time we return
       * we know that we have found the right task.
       *
       * @see ProcessReadyTask::processTasks() to trigger the actual lazy
       *      fusion of tasks.
       * @see ProcessReadyTask::hasTaskBeenProcessed() which is used to check
       *      if a particular task has completed already.
       */
      void ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(const std::set<TaskNumber>& inDependencies);

      /**
       *
       *
       * # Implementation
       *
       * If the task is ready, somebody could have triggered the computation
       * instead of us. So we issue a consumer task now, as we may assume that
       * there are many other tasks in there as well which are ready.
       *
       * After that, we while loop and wait until we know that the task is
       * complete.
       *
       */
      void ensureHeldBackTasksAreMappedOntoNativeTasks_LazyConsumerTasks(const std::set<TaskNumber>& inDependencies);
      void ensureHeldBackTasksAreMappedOntoNativeTasks_EagerConsumerTasks(const std::set<TaskNumber>& inDependencies);
    } // namespace taskfusion
  }
}
