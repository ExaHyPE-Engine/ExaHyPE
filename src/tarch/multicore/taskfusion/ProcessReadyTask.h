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
 * tasks.
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
 * process all tasks from the queue in one rush. The
 * task for which we have originally been created for through
 * tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence() must be
 * among the tasks in the queue, unless some other instance of ProcessReadyTask
 * has already handled it. If this is the case, we are fine. Let's do all the
 * other tasks that are in there and we are done.
 */
class tarch::multicore::taskfusion::ProcessReadyTask: public tarch::multicore::Task {
  private:
    ReadyFusableTasks* _taskQueue;
    const int          _taskNumber;

    static const std::string ProcessFusedTasksStatisticsIdentifier;

    static tarch::logging::Log _log;

    /**
     * Look up if the task number to which this process task is tied to has already been done
     *
     * This routine is thread-safe.
     */
    bool hasTaskBeenProcessed() const;

    /**
     * Omnipotent operation, i.e. you can call it as often as you want.
     *
     * This routine is thread-safe.
     */
    void removeProcessedTask();

    static void fuse(
      ReadyFusableTasks* taskQueue,
      int maxTasks,
      int device
    );

    /**
     * Search for the one task we are waiting for. If you find it, process it.
     */
    static void tryToProcessOneTask(
      ReadyFusableTasks* taskQueue,
      int                taskNumber
    );
  public:
    /**
     * Construct process ready task
     *
     * Construct a task that has to ensure that the task identified by
     * taskNumber is completed. If taskNumber equals NoDependency, then
     * the task has to ensure that the set of fused tasks is empty before
     * it returns.
     */
    ProcessReadyTask( int taskNumber, ReadyFusableTasks* taskQueue );

    /**
     * Run the task
     *
     *
     */
    virtual void run() override;

    /**
     * Process tasks from queue
     *
     * Very simple case distinction: We check if the queue is ready to fuse
     * a couple of tasks. If so, we do this. Otherwise, we invoke tryToProcessOneTask(),
     * i.e. we search for the specific task taskNumber and try to complete
     * this one.
     */
    static void processTasks(
      ReadyFusableTasks* taskQueue,
      int                taskNumber = -1
    );
};

