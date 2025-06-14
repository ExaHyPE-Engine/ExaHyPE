// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/taskfusion/taskfusion.h"


#include <map>
#include <unordered_set>


namespace tarch {
  namespace multicore {
    namespace taskfusion {
      class FusableTasksQueue;
    }
  }
}


class tarch::multicore::taskfusion::FusableTasksQueue {
  public:
    static const int AnyTask = -1;

    FusableTasksQueue(int type_);
    ~FusableTasksQueue()                        = default;
    FusableTasksQueue(const FusableTasksQueue&) = delete;

    void insertPendingTask( int taskNumber );

    /**
     *
     * ## Assertions
     *
     * As the task taskNumber has been inserted through insertPendingTask()
     * before, we know that it cannot be among the completed tasks yet.
     */
    void movePendingTaskIntoReadyTaskQueue( Task* task, int taskNumber );

    /**
     * Insert a new ready task into the local queues
     *
     * Is used by run() but we might also want to use it externally.
     * Before we insert the actual task, we also remove the task's
     * completedTaskNumbers, so noone thinks ahead of time that we are
     * done already.
     */
    void insertReadyTask( Task* task, int taskNumber );

    /**
     * Search for the one task we are waiting for. If you find it, process it.
     *
     * @param taskNumber Task number that has to be processed. If you pass in
     *   AnyTask, then the routine simply takes the first task (if there is one) and
     *   finishes that one.
     */
    void processOneTask(
      int                taskNumber
    );

    void drainReadyTasks();

    /**
     * Process tasks from queue and fuse them
     *
     * Very simple implementation running through a few basic checks:
     *
     * - Get the instructions for the task type at this point and also the
     *   size of the queue.
     * - If we are ready to fuse, extract all the tasks to be fused from the
     *   queue and hand them over to the fused routine.
     * - Bookkeep that these tasks are now completed.
     *
     * If there are not enough tasks to be fused, the routine degenerates to
     * nop.
     *
     * ## Threads-safety
     *
     * This routine is totally thread-safe
     */
    void processTasks();

    /**
     * Look up if the task number to which this process task is tied to has already been done.
     *
     * ## Thread safety
     *
     * This routine is totally thread-safe.
     *
     * @param tryLock If set, we use a try lock (not a lock) and return false
     *                if the try fails. If it is unset, we use a proper lock.
     */
    bool isTaskPending(
      int                 taskNumber,
      bool                tryLock
    );

    int getType() const;

    /**
     * Each task in the queue is identified through a pointer to the
     * task plus a task (dependency) number.
     */
    using Tasks                = std::map< int, tarch::multicore::Task* >;
    using TaskNumbers          = std::unordered_set< int >;

    /**
     * Tasks that are ready to go
     */
    Tasks                                      readyTasks;

    /**
     * Tasks which are currently tied to a LogTask instance but are not
     * yet ready
     */
    TaskNumbers                                pendingTasks;

    /**
     * Tasks that are complete
     */
    TaskNumbers                                runningTasks;

    tarch::multicore::BooleanSemaphore         pendingTasksSemaphore;
    tarch::multicore::BooleanSemaphore         readyTasksSemaphore;
    tarch::multicore::BooleanSemaphore         runningTasksSemaphore;

    tarch::multicore::BooleanSemaphore         statisticsSemaphore;
    tarch::timing::GlidingAverageMeasurement   numberOfPendingReadyTasksPerProcessTaskExecution;
    tarch::timing::GlidingAverageMeasurement   probabilityThatTaskIsCompleted;

  private:
    static const std::string                   ProcessFusedTasksStatisticsIdentifier;

    const int                                  _type;
};
