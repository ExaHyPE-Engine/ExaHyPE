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
     *
     *
     * ## Realisation of different task fusion strategies
     *
     * The different task fusion strategies are sketched out in
     * tarch::multicore::strategies::FusionImplementation. The text
     * there discusses how the task fusion acts and reacts. At their core,
     * all realisations however build upon the FusableTaskQueue, i.e. the
     * task queue offers a certain set up operations. The different fusion
     * strategies call/use these operations in different ways.
     *
     */
    namespace taskfusion {
      /**
       * Track how many tasks are directly inserted into ready queue
       *
       * The counter tracks
       * those tasks where the runtime things they should be fused. As the
       * tasks are already ready, it would be harakiri to run through instances
       * of LogReadyTask. Instead, we can insert those tasks directly into the
       * queue of fusable tasks.
       *
       * This counter is the direct counterpart to
       * @ref tarch::multicore::taskfusion::LogReadyTask::SubmitFusedTasksStatisticsIdentifier.
       * Please study this flag for further information including hints how to
       * visualise the outcome.
       */
      extern const std::string DirectlyInsertFusedTaskIntoReadyQueueStatisticsIdentifier;

      /**
       * Track number of fused tasks that are mapped directly onto native tasks
       *
       * The orchestration might decide to map fusable tasks onto native tasks
       * for various reasons. All of them are discussed in the documentation of
       * handleFusableTask(). If we assess the task fusion, it is a problem if
       * the task fusion actually never kicks in, i.e. if all tasks end up as
       * native tasks.
       *
       * @image html documentation/tarch/statistics-flawed-handling-of-tasks.png
       *
       * Visualising this counter flags the underlying problem. There's also a
       * longer discussion on @ref documentation_tarch_task_fusion_strategies
       * which refers to the pic above (where we have lots of fusable tasks, but
       * all of them end up being mapped onto native ones).
       */
      extern const std::string MapFusedTaskOntoNativeTaskStatisticsIdentifier;

      /**
       * Track number of tasks that are executed directly
       *
       * This is an even stronger flavour of MapFusedTaskOntoNativeTaskStatisticsIdentifier,
       * where tasks are not even mapped onto native tasks but actually
       * executed straightaway.
       *
       * Consult
       * @ref tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately
       * as well as the documentation handleFusableTask(). This counter/task
       * processing scheme should be used with uttermost care, as it potentially
       * breaks the tasking semantics.
       */
      extern const std::string ProcessFusedTaskDirectlyStatisticsIdentifier;

      struct FusableTasksQueue;

      /**
       * Handle one fusable task
       *
       * This operation handles one fusable task in the sense that it first
       * ensures that all infrastructure for the fusion is in place and that
       * we can, in principle, fuse. After
       * that, it delegates to implementation selected by the orchestration:
       *
       * 1. Ensure that all incoming tasks (predecessors) are mapped onto
       *    native tasks. Our implementation leaves all dependency tracking
       *    to the native runtime, and therefore, we invoke
       *    taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks()
       *    first.
       * 2. Find out if task fusion is active. If not, ensure that the task
       *    is either executed directly or spawned into a native task. Which
       *    variant to pick depends on the orchestration's getFusionImplementation()
       *    result, which returns an instance of tarch::multicore::orchestration::FusionImplementation.
       * 3. Lock the map over all queues through pendingTasksSemaphore.
       * 4. Ensure that a queue does exist for the particular task type.
       * 5. Invoke the task handling that's specified through getFusionImplementation().
       *
       * The routine is invoked by tarch::multicore::spawnTask() if this
       * routine is given a task that can, in theory, be fused. Below are some
       * remarks on particular strategies:
       *
       *
       * ## DisableFusionAndProcessImmediately and other degenerated strategies
       *
       * This variant from tarch::multicore::orchestration::FusionImplementation
       * implies that we directly call the task's run() operation. However, we
       * first have to ensure that we wait for all incoming tasks. We do so
       * through a call to waitForTasks(). This wait in return is valid as we
       * have previously called taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks().
       *
       *
       * ## LazyEvaluation
       *
       * Almost trivial realisation of the core functionality through
       * handleFusableTask_LazyEvaluation(). The interesting stuff, i.e. the
       * fusion and task processing, all happens in ensureHeldBackTasksAreMappedOntoNativeTasks()
       * which in turn calls ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation().
       *
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
       *
       * This routine invokes tarch::multicore::taskfusion::FusableTasksQueue::drainReadyTasks().
       * That is, this routine does not(!) issue any task fusion.
       */
      void processAllReadyTasks();

      /**
       * Drain the queues but use task fusion where possible
       *
       * This is the cousin to processAllReadyTasks(). It ensures that the task
       * queues are drained eventually, but it relies on task fusion rather
       * than a manual drain. Different to processAllReadyTasks(), you do not
       * have to issue a manual barrier afterward this routine has returned.
       * You may assume that all tasks are literally complete.
       *
       * This blocking behaviour implies that you stop your program execution.
       * However, nothing stops you, subject to the correct dependency
       * management, to deploy the call to processAndFuseAllReadyTasks() to yet
       * another task of its own.
       *
       * The routine can be used to set off timed execution as discussed in
       * tarch::multicore::orchestration::FusionImplementation.
       *
       * ## Implementation
       *
       * Calling waitForTask() will handle native tasks, but also will
       * eventually lead back to a call of ensureHeldBackTasksAreMappedOntoNativeTasks()
       *
       *
       *
       */
      void processAndFuseAllReadyTasks();

      /**
       * Realisation for handleFusableTask() for ProducerConsumerTaskSequence
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
      void handleFusableTask_ProducerConsumerTaskSequence(
        Task*                  task,
        FusableTasksQueue&     taskQueue,
        std::set<TaskNumber>   inDependencies,
        const TaskNumber&      taskNumber,
        const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction
      );

      /**
       * Implementation of handleFusableTask() if we use LazyEvaluation.
       *
       * Lazy evaluation means that we take a task, but we wrap it out for a
       * proxy task. This is an instance of LogReadyTask. The run() of the
       * LogReadyTask instance dumps the actual task into the respective
       * task queue. If there are no incoming dependencies, we can directly
       * dump the actual task.
       *
       * @image html src/tarch/multicore/taskfusion/taskfusion-LazyEvaluation.png
       *
       * After that, we can return. If someone waits for our task (which we
       * replaced with the LogReadyTask), we have to ensure that the task
       * has been removed from the queue and processed. This is realised
       * through ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation().
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
       * Handle a task in the eager consumer pattern
       *
       * This routine has to be used in combination with handleFusableTask_LazyEvaluation().
       * It does, different as the name suggests, not enqueue any task. Instead, it simply
       * evaluates if the task queue is ready for a fusion task. If so, it spawns one.
       *
       * This consumer of tasks can be totally independent of the actual task
       * Number. The only important thing is that it has a valid task number.
       * So if we wait for a task later, we have to be able to see that someone
       * potentially grabbed this one and have to wait for it.
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
       * @param taskNumbers: Set of task numbers we depend on. May not be
       *         empty.
       *
       * @return The routine might have created new native tasks, i.e. the
       *         calling routine has to wait for these native tasks.
       */
      bool ensureHeldBackTasksAreMappedOntoNativeTasks(const std::set<TaskNumber>& taskNumbers);

      /**
       * Preamble of waitForTasks() and task spawns if we use LazyEvaluation.
       *
       * The routine ensureHeldBackTasksAreMappedOntoNativeTasks() delegates to
       * this one if we have chosen lazy evaluation. The job here is that we
       * ensure that any pending work is mapped properly onto native tasks or
       * that all tasks have finished. In the latter case, any follow-up wait
       * degenerates to an anti-dependency.
       *
       * @image html src/tarch/multicore/taskfusion/taskfusion-LazyEvaluation.png
       *
       * The spawning of our task of interest has either created a LogReadyTask
       * instance, or it has directly dropped the task that we wait for into
       * the task queue. We don't know if this LogReadyTask is still out
       * there, so first we have to wait for it. We know it had the same number
       * as the original task, as we had swapped it out for this one.
       *
       * Next, we check if our task of interest is still in this queue, i.e. is
       * ready or running. We can do so through
       * anyTasksOfInDendenciesMightBeStillReadyOrRunning(). If they are not
       * there, we can return with a false, as we know that the calling routine
       * does not have to wait for us. We are ready to go.
       *
       * If one of our tasks that we wait for is still in the queue, we loop
       * over each individual in-dependency:
       *
       * - If the in-dependency is not found, skip and go to the next one.
       * - If the in-dependecy's task is in the queue, first issue a task which
       *   does all the task fusion (task A).
       * - Next, trigger another task B which picks only this one task we are
       *   actually interested in (task B).
       *
       * We ensure that A and B have exactly the same number of one of the
       * in-dependencies. This way, A will execute before B and the user will
       * wait for B. Please read the text below to understand why the split-up
       * into A and B is not just for fun. It is required to ensure that we
       * do not run into deadlocks.
       *
       *
       * ## Realisation
       *
       * We don't know, at this point, what the type of each in dependency is.
       * Therefore, we loop over all queues, i.e. all task types that we
       * are aware of, and look if any of the tasks we are interested in is
       * still in there labelled as ready or running.
       *
       * First, we look in each queue if it hosts the a task that is ready or
       * running. If this is the case, we invoke the task fusion and afterwards
       * ensure that the task is really completed by spawning
       * ProcessOneReadyTask.
       *
       *
       * ## Potential deadlocks
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
       * A flaw of the present implementation is that additional
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~
       * native::waitForTasks(inDependencies);
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * We need this one, as we don't know if there are still instances of
       * LogReadyTask out there pending. If we knew that this is not the case,
       * we could skip this step and eliminate one synchronisation point.
       *
       *
       * @see ProcessSetOfReadyTasks which triggers the actual lazy fusion of tasks.
       * @see ProcessOneReadyTask which is used to check if a particular task has completed already.
       */
      bool ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(const std::set<TaskNumber>& inDependencies);

      /**
       * Preamble of waitForTasks() if we use EagerEvaluation.
       *
       * This routine is very similar to ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation(),
       * besides the fact that it never ever spawns a fusion task. If the eager
       * scheme results in fusion tasks, then these fusion tasks have already
       * been created by the time we spawned the tasks. We don't create them by
       * the time we wait for a task. So we should say: "it is a degenerated
       * version of ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation()".
       */
      bool ensureHeldBackTasksAreMappedOntoNativeTasks_EagerConsumerTasks(const std::set<TaskNumber>& inDependencies);
    } // namespace taskfusion
  }
}
