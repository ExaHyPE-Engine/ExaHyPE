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
      class ProcesseOneReadyTask;
      struct FusableTasksQueue;
    }
  }
}


/**
 * Process one ready tasks
 *
 * @see ProcessSetOfReadyTasks for further implementation remarks
 * @see FusionImplementation::ProducerConsumerTaskSequence for context
 */
class tarch::multicore::taskfusion::ProcessOneReadyTask: public tarch::multicore::Task {
  private:
    FusableTasksQueue& _taskQueue;
    const TaskNumber   _taskNumber;

    static tarch::logging::Log _log;

  public:
    /**
     * Construct task
     *
     */
    ProcessOneReadyTask(
      TaskNumber          taskNumber,
      FusableTasksQueue&  taskQueue
    );

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
     * If the task is handled by an instance of ProcessSetOfReadyTasks,
     * we wait for this one.
     *
     * If neither of the two cases hold, we'll only process the one single tasks that
     * we are responsible for
     * (and therefore will afterwards terminate the while loop). But we might
     *
     * Ensure one particular task is completed.
     *
     * Search for the one task we are waiting for. If you find it, process it.
     * It could be that we don't have this task anymore among the ready tasks but
     * that it is currently merged into a bigger fusable task set and processed.
     * In this case, we have to issue a native wait on the task that handles the
     * actual fusion.
     *
     * @see Consult the page @ref documentation_tarch_task_fusion_strategies
     *   for recommendations and hints on task fusion strategies.
     * @see Consult the namespace documentation of tarch::multicore::taskfusion
     *   for details on the realisation of the task fusion.
     */
    virtual void run() override;
};

