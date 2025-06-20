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
      class ProcessReadyTask;
      struct FusableTasksQueue;
    }
  }
}


/**
 * Process ready task
 *
 * Each ProcessReadyTask instance is paired up with an instance of LogReadyTask
 * which runs before. That is, we know that an instance of LogReadyTask has put
 * the task of interest into the ready queue identified by _taskQueue. We
 * therefore can now lock this queue and start the processing of the fused
 * tasks and then pick our particular task that we are responsible for. This way,
 * we combine the fusion with an urgent return.
 *
 * However, it could happen that another process tasks has already grabbed the
 * task we are responsible for yet has not finished processing this one yet.
 * In this case, we try again until our key task is finally done.
 *
 *
 * ## Task that we are responsible has no outgoing dependency
 *
 * In this case, we know that there's some other synchronisation mechanism
 * somewhere such as some database polling. We fuse all tasks that are still
 * pending and return. We might return too early, as the task the user is
 * actually interested in has been grabbed by another instance of
 * ProcessReadyTask, but this is none of our problems.
 *
 *
 * ## Statistics
 *
 * If we study the task queue and our task of interest is already completed,
 * we return immediately. If this is not the case, we process tasks as described
 * above, but we also bookkeep how many tasks we currently see in the queue.
 *
 * Such intelligence is used by tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence()
 * to determine how quickly the task queues should be filled.
 */
class tarch::multicore::taskfusion::ProcessReadyTask: public tarch::multicore::Task {
  private:
    FusableTasksQueue& _taskQueue;
    const int          _taskNumber;

    static tarch::logging::Log _log;

  public:
    /**
     * Construct process ready task
     *
     * Construct a task that has to ensure that the task identified by
     * taskNumber is completed. If taskNumber equals NoDependency, then
     * the task has to ensure that the set of fused tasks is empty before
     * it returns.
     *
     * @param taskNumber Pass in a bespoke task number. If this task is
     *   already complete when the task kicks off in run(), then the process
     *   task will return immediately. You can pass in
     *   tarch::multicore::taskfusion::FusableTasksQueue::AnyTask as well to
     *   disable this.
     */
    ProcessReadyTask( int taskNumber, FusableTasksQueue& taskQueue );

    /**
     * Run the task
     *
     * Will return if and only if the task with taskNumber is completed.
     * We ensure this by calling processTasks() which in turn invokes
     * tryToProcessOneTask().
     *
     * If the task we are tied to is already completed - a property we
     * can check through hasTaskBeenProcessed() - we return immediately
     * and nothing is to be done. Another ProcessReadyTask instance has
     * snapped "our" task and we can degenerate.
     *
     * If the task is not yet complete, we checks if there's potential to fuse.
     * If so, we call processTasks(). The check for a successful fuse can be
     * done unsafely, as processTasks() will check once again, i.e. ensure
     * that noone has grabbed tasks meanwhile.
     *
     * We we cannot fuse but should fuse and if we are told to use a timeout,
     * we enter the timeout phase, i.e. we wait for a specified time if more
     * tasks become available to fuse.
     *
     * If we cannot fuse anymore, we'll only process the one single tasks that
     * we are responsible for
     * (and therefore will afterwards terminate the while loop).
     *
     * @see Consult the page @ref documentation_tarch_task_fusion_strategies
     *   for recommendations and hints on task fusion strategies.
     * @see Consult the namespace documentation of tarch::multicore::taskfusion
     *   for details on the realisation of the task fusion.
     */
    virtual void run() override;
};

