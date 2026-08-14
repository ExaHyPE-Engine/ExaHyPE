// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/TaskEnumerator.h"
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
 * Wrapper task around fusable tasks that is not ready yet
 *
 * This task takes a fusable task and "inherits" its in-dependencies. Once it
 * becomes ready ready, it takes its fusable task and inserts this one into the
 * ready queue. The task therefore is a mere wrapper: It ensure that a fusable
 * task is not inserted into the ready queue before it really becomes ready.
 * Other than that, it does not do anything.
 *
 * ## Lazy evaluation strategies
 *
 * If the fusion is implemented lazily, the spawning of a new fusable task with
 * in-dependencies should lead to the creation of this task. The fusable routine
 * then has to wait before the lazy evaluation is due, so we know that the right
 * tasks are inserted.
 *
 * ## Orchestration
 *
 * This task is always paired up with an instance of ProcessReadyTask. This
 * task takes the task _task and puts it into the _taskQueue. After that, it
 * quits, i.e. logically hands over the responsibility to ProcessReadyTask.
 */
class tarch::multicore::taskfusion::LogReadyTask: public tarch::multicore::Task {
  private:
    Task*               _task;
    const int           _taskNumber;
    FusableTasksQueue&  _taskQueue;

    const bool          _administerYourOwnTemporaryTaskLocalNumber;

    /**
     * Statistics identifier
     *
     * This statistics identifier tells you how many tasks have actually been
     * submitted through a LogReadyTask. If fusable tasks hit the runtime and
     * are ready already, we insert them straight. If they are not ready yet,
     * they are given in custody of a LogReadyTask which dumps them later. To
     * analyse this flag, you can follow the recipe of
     * @ref tarch::multicore::SpawnedTasksStatisticsIdentifier.
     */
    static const std::string SubmitFusedTasksStatisticsIdentifier;

    static tarch::logging::Log _log;

    static tarch::multicore::TaskEnumerator _enumerator;
  public:
    static constexpr TaskNumber AdministerYourOwnTemporaryTaskLocalNumber = -3;

    /**
     * Construct log read task
     *
     * @param task Task that is to be inserted into fusable queue as soon as
     *             this task starts to run
     * @param taskNumber out dependency (no) of outgoing task
     * @param taskQueue  Reference to task queue
     */
    LogReadyTask( Task* task, TaskNumber taskNumber, FusableTasksQueue& taskQueue );

    /**
     * Run task
     *
     * If this routine is invoked, we know that all the incoming dependencies
     * of _task are now complete. We refrain from running the task now (even
     * though it would be valid). Instead, we lock our queue and add the task.
     * handleFusableTask() should already have issued a
     * follow-up task of type ProcessReadyTask. Therefore, this is all to be
     * done here. No more work.
     */
    virtual void run() override;

    TaskNumber getNumber() const;
};
