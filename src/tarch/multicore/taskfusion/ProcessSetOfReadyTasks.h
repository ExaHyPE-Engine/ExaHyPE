// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/taskfusion/taskfusion.h"


namespace tarch {
  namespace multicore {
    namespace taskfusion {
      class LogReadyTask;
      class ProcessSetOfReadyTasks;
      struct FusableTasksQueue;
    }
  }
}


/**
 * Process set of ready tasks
 *
 * Process ready tasks is a class which grabs a set of ready tasks and finishes
 * them. The clue here is that this task itself is again tied to one fusable
 * task and that this task might change its internal implementation:
 *
 * - If the underlying fusable task has completed already, this task terminates
 *   immediately.
 * - If there are lots and lots of ready tasks, this task will grab them
 *   and and complete those guys.
 *
 *
 * ## Performance
 *
 * If we benchmark
 * FusionImplementation::ProducerConsumerTaskSequence, then our performance will
 * likely suffer quite a lot. We should, in practice avoid to have to many of
 * these ProcessSetOfReadyTasks instances.
 *
 *
 * ## Usage patterns
 *
 * If a task has in-dependencies, then
 * each ProcessSetOfReadyTasks instance is always paired up with an instance of LogReadyTask
 * which runs before. That is, we know that an instance of LogReadyTask has put
 * the task of interest into the ready queue identified by _taskQueue. This way,
 * fusion does not kick in prior to the actual task depositing.
 *
 *
 *
 * ## Implementations (with historic information on bugs)
 *
 * I originally built in a guarantee that the termination of this task also
 * means that the fusable task with the same task number has terminated. This
 * can lead to nasty deadlocks. Assume that we have two instances of
 * ProcessSetOfReadyTask t1 and t2 and t1 grabs some tasks A,B,C,D, while
 * t2 takes E,F,G,H. If t2 now waits for A as it is tied to A and t1 is tied
 * to H, then we have a deadlock.
 *
 * So what I do is that I separate the fusion handling from the tackling of
 * an individual task, i.e. ProcessSetOfReadyTasks and ProcessOneReadyTask are
 * distinct tasks with different behaviour: One takes an arbitrary number of
 * fusable tasks but there can be many of them. The other one takes a specific
 * task but only one of them.
 *
 * When either of them takes ownership of a fusable task, it registers itself
 * as owner. Therefore, no cross-dependencies aka deadlocks can arise anymore.
 *
 * This deadlock is also the reason why we work with three different task types
 * in general, and we avoid that they are merged into each other.
 *
 * @image html multicore_task_flow-01.png
 *
 *
 * ## Statistics
 *
 * If we study the task queue and our task of interest is already completed,
 * we return immediately. If this is not the case, we process tasks as described
 * above, but we also bookkeep how many tasks we currently see in the queue.
 *
 * Such intelligence can be used for decision making or autotuning, but atm it
 * is not exploited in any way.
 */
class tarch::multicore::taskfusion::ProcessSetOfReadyTasks: public tarch::multicore::Task {
  private:
    FusableTasksQueue& _taskQueue;
    const int          _taskNumber;

    static tarch::logging::Log  _log;
    static const std::string    ProcessFusedTasksStatisticsIdentifier;

  public:
    /**
     * Construct process ready task
     *
     * The ready task should correspond to a proper task number. I usually
     * pick a number of a fused task. As the fused task is parked somewhere in
     * the queues, we know this number is available. But it doesn't have to be.
     * The important thing just is that this task number is the same one that
     * we later on use for spawnTask(), as we take this number and put it next
     * to running tasks in run().
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Task* processTask = new ProcessSetOfReadyTasks(myTaskNumber, taskQueue);
     * tarch::multicore::native::spawnTask( processTask, tarch::multicore::NoInDependencies, myTaskNumber);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * @param taskNumber     Task number that you also used for spawnTask().
     */
    ProcessSetOfReadyTasks(
      TaskNumber          taskNumber,
      FusableTasksQueue&  taskQueue
    );

    /**
     * Run the task
     *
     * If the fusable task we are tied to is already completed - a property we
     * can check through hasTaskBeenProcessed() - we return immediately
     * and nothing is to be done. Another ProcessSetOfReadyTasks instance has
     * snapped "our" task and we can degenerate.
     *
     * If the task is not yet complete, we check if there's potential to fuse
     * and trigger the fused handling.
     *
     *
     * ## Fusion process
     *
     * Very simple implementation running through a few basic checks:
     *
     * - Get the instructions for the task type at this point and also the
     *   size of the queue.
     * - If we are ready to fuse, extract all the tasks to be fused from the
     *   queue and hand them over to the fused routine.
     * - Meanwhile, put this task's task number next to running fusable task.
     *   If someone else needs the outcome of the fusable task, they can wait
     *   for this task until we are done.
     *
     * If there are not enough tasks to be fused, the routine degenerates to
     * nop.
     *
     *
     * ## Thread-safety
     *
     * This routine is totally thread-safe.
     *
     *
     * ## Implementation remarks
     *
     * - I check the semaphore with LockQueueAndReturnReliableResult. The
     *   fusion and the follow-up steps are not cheap. So it is better to be
     *   conservative here and to introduce locking overhead rather than to
     *   jump into the fusion prematurely.
     *
     *
     * @see Consult the page @ref documentation_tarch_task_fusion_strategies
     *   for recommendations and hints on task fusion strategies.
     * @see Consult the namespace documentation of tarch::multicore::taskfusion
     *   for details on the realisation of the task fusion.
     */
    virtual void run() override;
};

