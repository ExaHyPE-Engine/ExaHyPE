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
     * different strategies are controlled through the multicore orchestration.
     * There's the enum tarch::multicore::orchestration::FusionImplementation
     * in there, which also contains all the high-level overviews of how the
     * different strategies work and when they are appropriate.
     * More information on how to use the task fusion and how to tailor it is
     * provided in the context of @ref documentation_tarch_task_fusion_strategies.
     * On this page, we do a deep-dive into the implementation of the various
     * strategies.
     *
     *
     * ## Task queues
     *
     * To ensure that fusion is only applied to tasks of the same type, we store
     * the fusable tasks in a map indexed by the task type. The map maps each
     * task type onto an instance of tarch::multicore::taskfusion::FusableTaskQueue.
     * When you spawn a task through handleFusableTask(),
     * the first thing this routine has to do is to check if there's a task
     * queue in place already. Consult the routine's description for more
     * details.
     *
     * The two variables that are used are therefore
     *
     * - fusableTaskQueue a map of task types onto queue instances on the heap, and
     * - pendingTasksSemaphore which is a sempahore to protect access to the heap.
     *
     * The two variables are only found in the implementation file, as they are
     * not visible from outside. As our map works with pointers to the actual
     * queues, it is sufficient to have one semaphore that protects access: Once
     * we have got the pointer from the map, we can always release the semaphore
     * and work against the queue object on the heap. Meanwhile, other routines
     * might add new queues to the map. We don't really care anymore.
     *
     *
     * ## The important routines
     *
     * There are only two important routines from a user's point of view:
     *
     * - handleFusableTask() is the spawn mechanism which is used by
     *   spawnTask() to hand over a task that  can, in theory, be fused.
     * - ensureHeldBackTasksAreMappedOntoNativeTasks() takes a couple of
     *   tasks and ensures they are mapped onto proper native tasks.
     * - processAllReadyTasks() is a drain mechanism over all queues.
     *
     * Consult the documentation of these routines for details when they
     * are used. The documentation also provides information how the
     * underlying implementations work.
     */
    namespace taskfusion {
      struct FusableTasksQueue;

      /**
       * Handle one fusable task
       *
       * This operation handles one fusable task in the sense that it first
       * ensures that all infrastructure for the fusion is in place. After
       * that, it delegates to implementation selected by the orchestration:
       *
       * 1. Find out if task fusion is active. If not, ensure that the task
       *    is either executed directly or spawned into a native task. Which
       *    variant to pick depends on the orchestration's getFusionImplementation()
       *    result, which returns an instance of tarch::multicore::orchestration::FusionImplementation.
       * 2. Lock the map over all queues through pendingTasksSemaphore.
       * 3. Ensure that a queue does exist for the particular task type.
       * 4. Invoke the task handling that's specified through getFusionImplementation().
       *
       * The routine is invoked by tarch::multicore::spawnTask() if this
       * routine is given a task that can, in theory, be fused.
       *
       * @see Page @ref documentation_tarch_task_fusion_strategies provides a high-level overview
       * @see tarch::multicore::orchestration::FusionImplementation enlists the different implementation variants
       */
      void handleFusableTask(
        Task*                  task,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber
      );

      /**
       * Run over all known queues and invoke drainReadyTasks() on each one.
       *
       * Once we know that all fusion queues are drained, we know that they
       * are either complete or mapped successfully onto native tasks. That
       * is, we can invoke a task barrier.
       *
       * This routine is used by the global synchronisation, i.e. by
       * tarch::multicore::waitForAllTasks(), but we also have to invoke it
       * before we yield: The whole idea behind a yield() is that we give
       * the runtime the opportunity to bring in some other tasks, i.e. to
       * disrupt the calculation. However, the runtime cannot bring in other
       * tasks if the tasks are hidden away in a queue.
       */
      void processAllReadyTasks();

      /**
       * Realisation for handleFusableTask() for ProducerConsumerTaskPairs
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
      void handleFusableTask_ProducerConsumerTaskPairs(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       * Implementation of handleFusableTask() if we use LazyEvaluation.
       *
       * @see @ref documentation_tarch_task_fusion_strategies for some context for this routine.
       */
      void handleFusableTask_LazyEvaluation(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       *
       */
      void handleFusableTask_EagerConsumerTasks(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       * Ensure that a certain set of tasks is mapped onto native tasks already
       *
       * Our task fusion holds back tasks in bespoke queues such that they can
       * be bundled together into a larger meta task. However, this implies
       * that we cannot do two things:
       *
       * 1. We cannot wait for them with built-in waits(). If a task, for
       *    example, is not yet passed on to OpenMP, OpenMP's wait cannot
       *    wait for this task by definition.
       * 2. If we have in-dependencies of a new task, these in-dependencies
       *    cannot be modelled with on-board features if the predecessor is
       *    not yet mapped onto a task. We would create an antidependency.
       *
       * In both cases, we therefore have to invoke this routine first, so we
       * ensure that the tasks that we wait for or depend on, respectively, is
       * indeed already a native task and not still parked in a queue
       * somewhere.
       *
       * The routine delegates to implementation-specific variants depending
       * on which task fusion strategy the orchestration picks.
       *
       * The function is used by tarch::multicore::waitForTasks() and
       * tarch::multicore::spawnTask(). The latter calls it over all incoming
       * dependencies.
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
       * handleFusableTask_EagerConsumerTasks() and
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
