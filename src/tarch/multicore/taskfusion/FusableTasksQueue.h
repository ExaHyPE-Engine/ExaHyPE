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
      class ProcessOneReadyTask;
      class ProcessSetOfReadyTasks;
    }
  }
}


/**
 * Represents a queue of fusable tasks
 *
 * Each queue object is tied to one particular task type. We can query this
 * type via getType().
 *
 * The task queue does not implement a particular task
 * fusion strategy, but is a mere tool used by various strategies. The
 * documentation of tarch::multicore::orchestration::FusionImplementation
 * provides an overview of these strategies. That means: the queue itself
 * offers a small set of operations, which is really all the tools you need
 * to actually construct a fusion. The different strategies then differ in the
 * way they use these operations, the order how they invoke them, and so forth.
 *
 *
 * ## Task states
 *
 * The queue distinguishes three different task states: pending, ready and
 * running. When we insert a task, we label it as either pending (with incoming
 * dependencies which are not yet fulfilled) or ready. If the tasking subsystem
 * manages to fullfill the in-dependencies of a pending task, it is the
 * responsibility of the task system to move the task within the queue from
 * pending to running through a call to movePendingTaskIntoReadyTaskQueue().
 *
 * If tasks are starting to run, we have to label them as running as well.
 * There are no routines for this, as the task queue is always paired up
 * with instances of ProcessOneReadyTask or ProcessSetOfReadyTasks. See remark
 * on the queue usage below.
 *
 *
 * ## Usage pattern
 *
 * We hand over a task to the task queue either by inserting
 * it directly into the task queue, or by telling the queue that a previously
 * pending task has now become ready. After that, we have given up ownership
 * of this fusable task. The ownership resides within the queue.
 *
 * However, we can construct instances of
 * ProcessOneReadyTask and ProcessSetOfReadyTasks over the task queue. They
 * are responsible for the lifecycle management within the queue and allow us
 * to work with the tasks that we have dumped into it.
 *
 * The decision when or which of these two tasks to construct over the queue
 * is influenced by the queue's status, i.e. isTaskPending(), isTaskReadyOrRunnint(),
 * and so forth, as well as by our chosen task fusion strategy. All strategies
 * handle the tasks within the queue by constructing new tasks of type
 * ProcessOneReadyTask and ProcessSetOfReadyTasks over the task queue. If they
 * wait for any of these tasks immediately, we have blocking semantics. As we
 * issue the tasks in certain orders and quantities, we can construct various
 * task processing patterns.
 *
 */
class tarch::multicore::taskfusion::FusableTasksQueue {
  public:
    static constexpr int AnyTask = -1;

    friend class ProcessOneReadyTask;
    friend class ProcessSetOfReadyTasks;


    FusableTasksQueue(int type_);
    ~FusableTasksQueue()                        = default;
    FusableTasksQueue(const FusableTasksQueue&) = delete;

    /**
     * Insert a new task that is pending
     *
     * Pending tasks are tasks that are not ready yet. At this point, it is
     * enough to know the number of this task, as we assume that the task
     * is an instance of LogReadyTask. That is: If the user wants to add a
     * new task with incoming dependencies, then the user has to create an
     * instance of LogReadyTask (with the same number as the fusable task)
     * and inform the queue about this task.
     */
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
     * Drain all the ready tasks
     *
     * This routine is used by tarch::multicore::taskfusion::processAllReadyTasks().
     */
    void drainReadyTasks();

    /**
     * Can be used with the queries below if you prefer those names over
     * the boolean values. Makes it clearer what a function does.
     */
    static constexpr bool UseTryLockAndReturnTrueIfLockFailed = true;
    static constexpr bool LockQueueAndReturnReliableResult    = false;

    /**
     * Look up if the task number to which this process task is tied to has already been done.
     *
     * ## Thread safety
     *
     * This routine is totally thread-safe.
     *
     * Use UseTryLockAndReturnTrueIfLockFailed or LockQueueAndReturnReliableResult
     * for the tryLock values if you are unsure.
     *
     * @param tryLock If set, we use a try lock (not a lock) and return false
     *                if the try fails. If it is unset, we use a proper lock.
     */
    bool isTaskPending(
      int                 taskNumber,
      bool                tryLock
    );

    bool isTaskReadyOrRunning(
      int                 taskNumber,
      bool                tryLock
    );

    bool isTaskReady(
      int                 taskNumber,
      bool                tryLock
    );

    int getType() const;

    /**
     * Not thread-safe, i.e. might return outdated information
     */
    int getNumberOfReadyTasks() const;
  private:
    static tarch::logging::Log                 _log;

    const int                                  _type;

    /**
     * Each task in the queue is identified through a pointer to the
     * task plus a task (dependency) number.
     */
    using ReadyTasks           = std::map< int, tarch::multicore::Task* >;

    /**
     * Map fusable task (number) onto the number of the ProcessReadyTask which actually
     * handles them.
     */
    using RunningTasks         = std::map< int, int >;

    using TaskNumbers          = std::unordered_set< int >;

    /**
     * Tasks that are ready to go
     */
    ReadyTasks                                 _readyTasks;

    /**
     * Tasks which are currently tied to a LogTask instance but are not
     * yet ready
     */
    TaskNumbers                                _pendingTasks;

    /**
     * Tasks that are currently running
     */
    RunningTasks                               _runningTasks;

    tarch::multicore::BooleanSemaphore         _pendingTasksSemaphore;
    tarch::multicore::BooleanSemaphore         _readyTasksSemaphore;
    tarch::multicore::BooleanSemaphore         _runningTasksSemaphore;

    tarch::multicore::BooleanSemaphore         _statisticsSemaphore;
    tarch::timing::GlidingAverageMeasurement   _numberOfPendingReadyTasksPerProcessTaskExecution;
    tarch::timing::GlidingAverageMeasurement   _probabilityThatTaskIsCompleted;
};
