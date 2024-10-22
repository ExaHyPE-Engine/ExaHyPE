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
 * Each ready task is paired up with an instance of LogReadyTask which runs
 * before. That is, we know that an instance of LogReadyTask has put the task
 * of interest into the ready queue identified by _taskQueue. We therefore can
 * now lock this queue and process all tasks from the queue in one rush. The
 * task for which we have originally been created for through
 * tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence() must be
 * among the tasks in the queue, unless some other instance of ProcessReadyTask
 * has already handled it. If this is the case, we are fine. Let's do all the
 * other tasks that are in there and we are done.
 */
class tarch::multicore::taskfusion::ProcessReadyTask: public tarch::multicore::Task {
  private:
    ReadyFusableTasks* _taskQueue;

    static const std::string ProcessFusedTasksStatisticsIdentifier;

    static tarch::logging::Log _log;
  public:
    ProcessReadyTask( ReadyFusableTasks* taskQueue );

    /**
     * Run the task
     *
     *
     */
    virtual bool run() override;
};

