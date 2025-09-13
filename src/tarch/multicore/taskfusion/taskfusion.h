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
     *
     * ## Some implementation lessons learned
     *
     * Originally, I thought we'd need a couple of these
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * while (queue.second->isTaskReadyOrRunning(taskNumber, false)) {
     *   queue.second->processTasks();
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * whenever we have to ensure that a task is done. After all, someone else
     * might just have used this one as part of task fusion. However, such a
     * while is toxic. It can always lead to deadlocks. Instead, every task has
     * to be mapped onto a task dependency, and we have to ensure that we can
     * properly wait for tasks. It is fine if a task is already done, and in
     * this case we can prune the DAG and omit a wait, but otherwise, we have
     * to ensure that we work with task dependencies.
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

      /**
       *
       */
      void translateFusableTaskIntoTaskSequence_EagerConsumerTasks(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       * If we run into a task wait, we have to be sure that fusable tasks are mapped onto proper tasks
       *
       * Delegates to implementation-specific variants.
       *
       * @return The routine might have created new native tasks, i.e. the
       *         calling routine has to wait for these native tasks.
       */
      bool ensureHeldBackTasksAreMappedOntoNativeTasks(const std::set<TaskNumber>& taskNumbers);

      /**
       * Preamble of waitForTasks() if we use LazyEvaluation.
       *
       * We don't know, at this point, what the type of each in dependency is.
       * Therefore, we loop over all queues, i.e. all task types that we
       * are aware of, and look if any of the tasks we are interested in is
       * still in there labelled as ready or running. This works, as we expect
       * the user to have called ensureHeldBackTasksAreMappedOntoNativeTasks_EnsureAllTasksAreRunningReadyOrHaveCompleted()
       * before, i.e. all in-dependencies are in that state.
       * We use a try-lock here to avoid blocking. If this try fails,
       * we have to assume that the task is still in the queue. If we assume
       * that at least one task is running or is ready, we have to continue to
       * do some work:
       *
       * First, we look in each queue if it hosts the a task that is ready or
       * running. If this is the case, we invoke the task fusion and afterwards
       * ensure that the task is really completed by spawning
       * ProcessOneReadyTask.
       *
       * It is important that ProcessOneReadyTask and ProcessSetOfReadyTasks
       * are modelled independently to avoid deadlocks:
       * In most cases, the two tasks just follow each other. However, it could also be that
       * we hit this routine on threads A and are being asked to process task
       * t1. At the same time, thread B also hits the same routine triggered
       * for task t2. As it happens B is faster and grabs t1. Now, if thread A
       * checks for t1 to be ready or running, the routine processTask() will,
       * rightly so, wait for the responsible task for t1 to finish its work.
       * That means, we have to embed the whole handling into a separate task,
       * so we can wait for it.
       *
       *
       * ## Concurrency level
       *
       * The routine seems to basically create two tasks that run after each
       * other and then tells the invoking code part to wait for it. So it
       * seems not to alter the concurrency level. However, it actually
       * increases it: If we wait for multiple input tasks, it issues these
       * pairs of tasks per unfulfilled in-dependency.
       *
       * Despite this observation, it remains clear that the eager invocation
       * yields a higher concurrency level. See
       * translateFusableTaskIntoTaskSequence_EagerConsumerTasks() and
       * ensureHeldBackTasksAreMappedOntoNativeTasks_EagerConsumerTasks()
       * for details.
       *
       *
       * @see ProcessReadyTask::processTasks() to trigger the actual lazy
       *      fusion of tasks.
       * @see ProcessReadyTask::hasTaskBeenProcessed() which is used to check
       *      if a particular task has completed already.
       */
      bool ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(const std::set<TaskNumber>& inDependencies);

      /**
       * Preamble of waitForTasks() if we use EagerEvaluation.
       *
       * This routine is very similar to ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation()
       * besides the fact that it never ever spawns a fusion task. If the eager
       * scheme results in fusion tasks, then these fusion tasks have already
       * been created by the time we spawned the tasks. We don't create them by
       * the time we wait for a task.
       */
      bool ensureHeldBackTasksAreMappedOntoNativeTasks_EagerConsumerTasks(const std::set<TaskNumber>& inDependencies);

      /**
       * Ensure tasks are not pending anymore
       *
       * If we want to ensure that a fusable task is mapped onto a proper task
       * (set) already, then we first of all have to check if a fusable task
       * is still pending, i.e. parked in the queue of tasks that are not
       * ready yet. So we have to ensure that they are not blocked. If they are
       * not yet among the ready tasks (or already running or have completed),
       * then we add a wait here.
       */
      void ensureHeldBackTasksAreMappedOntoNativeTasks_EnsureAllTasksAreRunningReadyOrHaveCompleted(const std::set<TaskNumber>& inDependencies);
    } // namespace taskfusion
  }
}
