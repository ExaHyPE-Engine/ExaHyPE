// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Strategy;
    }
  }
}


#include <utility>
#include <string>


/**
 * Interface for any task orchestration
 */
class tarch::multicore::orchestration::Strategy {
  public:
    /**
     * Provide hint of execution policy
     *
     * - EmbedWithoutSubTasks: Recommend to the tasking runtime that this
     *   task should be executed immediately without spawning a new task.
     * - MinimiseLatency: If you have a set of tasks that are to be
     *   executed en bloc, you make the recommendation that any thread
     *   which terminates before the others busy polls for them to
     *   terminate. That means that the system might suffer from
     *   underutilisation, but the algorithmic latency is low: As soon as
     *   the last task of a group has terminated, we continue immediately.
     * - MaximiseThroughput: Recommend that any thread that terminates its
     *   task early should immediately grab another task. So you might
     *   suffer from some increased latency - if a thread "accidentially"
     *   gets a huge task, it might not be available for a while once
     *   the loop has terminated. But this way, you maximise throughput as
     *   every task always tries to do a task all the time.
     *
     * @todo I would like to have a flag which tells the actual multicore
     *   runtime not (!) to continue with further ready tasks. Such a
     *   feature does not exist in OpenMP, e.g., and therefore we do not
     *   use such a flag.
     */
    enum class ExecutionPolicy {
      RunSerially,
      RunParallel,
      RunParallelAndIgnoreWithholdSubtasks
    };
    
    struct FuseInstruction {
      int  device;
      int  minTasks;
      int  maxTasks;
      FuseInstruction(int device_, int minTasks_, int maxTasks_):
        device(device_),
        minTasks(minTasks_),
        maxTasks(maxTasks_) {}

      std::string toString() const {
        return "(#device=" + std::to_string(device) + ",min=" + std::to_string(minTasks) + ",max=" + std::to_string(maxTasks) + ")";
      }
    };

    virtual ~Strategy() = default;

    /**
     * Notifies the strategy that we enter a BSP section
     */
    virtual void startBSPSection() = 0;

    /**
     *
     */
    virtual void endBSPSection()   = 0;

    /**
     * Tell the runtime system how many tasks to hold back: If there
     * are more tasks than the result, the tasking system will map them
     * onto native tasks. As long as we have fewer tasks than this
     * number, the runtime system will store tasks in its internal
     * queue and not pass them on. Holding tasks back gives us the
     * opportunity to fuse tasks, and it reduces pressure from the
     * underlying task system. It also is an implicit priorisation, i.e.
     * tasks that we hold back are ready yet have low priority.
     *
     * Is not const, as I want strategies give the opportunity to adopt
     * decisions after each call.
     */
    virtual int getNumberOfTasksToHoldBack() = 0;

    /**
     * How many tasks to fuse and to which device to deploy
     *
     * Return a triple modelled via a FuseInstruction object.
     *
     * - The first entry specifies which device to use to deploy the
     *   tasks to.
     *   You can also return tarch::multicore::Task::Host to indicate that
     *   this is a fused task that shall run on the host rather than a
     *   device.
     * - The second entry is the minimal number of tasks to fuse. If
     *   there are less than these tasks in the queue, don't fuse.
     * - The third entry is the maximum number of tasks to fuse. Never
     *   batch more than this count into one (meta) task.
     *
     */
    virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice() = 0;

    /**
     * If you set this flag, tasks are immediately fused. You can ensure
     * that only idling threads fuse tasks. In this case, return false.
     */
    virtual bool fuseTasksImmediatelyWhenSpawned() = 0;

    /**
     * Determine how to handle/realise parallelisation within fork/join region
     *
     * If you want threads to run through the queues and to process tasks,
     * then return true here. If you return true, the default behaviour is
     * that a traversal (grid sweep) task does not terminate when it has
     * completed its traversal, but instead polls the task queue. By
     * default, a task will always first try to process its local tasks.
     *
     * @param nestedParallelismLevel Please compare with tarch::multicore::spawnAndWait()
     *   which ensures that this flag equals 1 on the top level.
     *
     * @param taskType We assume that all numberOfTasks tasks are of the same
     *   type.
     */
    virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) = 0;
};

