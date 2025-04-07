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
 * Task plugging into state transition once task becomes ready
 *
 * This task is always paired up with an instance of ProcessReadyTask. This
 * task takes the task _task and puts it into the _taskQueue. After that, it
 * quits, i.e. logically hands over the responsibility to ProcessReadyTask.
 */
class tarch::multicore::taskfusion::LogReadyTask: public tarch::multicore::Task {
  private:
    Task*               _task;
    const int           _taskNumber;
    ReadyFusableTasks*  _taskQueue;

    static const std::string SubmitFusedTasksStatisticsIdentifier;

    static tarch::logging::Log _log;
  public:
    /**
     * Construct log read task
     *
     * @param task Task that is to be inserted into fusable queue as soon as
     *             this task starts to run
     * @param taskNumber out dependency (no) of outgoing task
     * @param taskQueue  Reference to task queue
     */
    LogReadyTask( Task* task, int taskNumber, ReadyFusableTasks* taskQueue );

    /**
     * Run task
     *
     * If this routine is invoked, we know that all the incoming dependencies
     * of _task are now complete. We refrain from running the task now (even
     * though it would be valid). Instead, we lock our queue and add the task.
     * translateFusableTaskIntoTaskSequence() should already have issued a
     * follow-up task of type ProcessReadyTask. Therefore, this is all to be
     * done here. No more work.
     */
    virtual void run() override;

    /**
     * Insert a new ready task into the local queues
     *
     * Is used by run() but we might also want to use it externally.
     * Before we insert the actual task, we also remove the task's
     * completedTaskNumbers, so noone thinks ahead of time that we are
     * done already.
     */
    static void insertReadyTask( Task* task, int taskNumber, ReadyFusableTasks* taskQueue );
};
