// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Strategy;
    } // namespace orchestration
  }   // namespace multicore
} // namespace tarch

#include <string>
#include <utility>

/**
 * Interface for any task orchestration
 *
 * There are multiple orchestration strategies implementing this interface and
 * hence guiding the task execution pattern. You can create those and make them
 * live by calling tarch::multicore::setOrchestration().
 */
class tarch::multicore::orchestration::Strategy {
public:
  /**
   * Provide hint of execution policy
   *
   * @see tarch::multicore::spawnAndWait() for rationale and details.
   * @see tarch::multicore for general overview and further design rationale.
   *
   * @todo I would like to have a flag which tells the actual multicore
   *   runtime not (!) to continue with further ready tasks. Such a
   *   feature does not exist in OpenMP, e.g., and therefore we do not
   *   use such a flag.
   */
  enum class ExecutionPolicy { RunSerially, RunParallel, RunParallelAndIgnoreWithholdSubtasks };

  struct FuseInstruction {
    int device;
    int minTasks;
    int maxTasks;
    FuseInstruction(int device_, int minTasks_, int maxTasks_):
      device(device_),
      minTasks(minTasks_),
      maxTasks(maxTasks_) {}

    std::string toString() const {
      return "(#device=" + std::to_string(device) + ",min=" + std::to_string(minTasks)
             + ",max=" + std::to_string(maxTasks) + ")";
    }
  };

  virtual ~Strategy() = default;

  /**
   * Notifies the strategy that we enter a BSP section
   */
  virtual void startBSPSection(int nestedParallelismLevel) = 0;

  /**
   *
   */
  virtual void endBSPSection(int nestedParallelismLevel) = 0;

  static constexpr int EndOfBSPSection = -1;

  /**
   * How many tasks shall system hold back from tasking runtime in user-defined queues
   *
   * Tell the runtime system how many tasks to hold back: If there
   * are more tasks than the result, the tasking system will map them
   * onto native tasks. As long as we have fewer tasks than this
   * number, the runtime system will store tasks in its internal
   * queue and not pass them on. Holding tasks back gives us the
   * opportunity to fuse tasks, and it reduces pressure from the
   * underlying task system. It also is an implicit priorisation, i.e.
   * tasks that we hold back are ready, but as we do not pass them
   * on to the tasking runtime, they implicitly have ultra-low
   * priority.
   *
   * My data suggest that it is a very delicate decision to hold back
   * tasks, as you run risk all the time that you starve threads even
   * though work would be available. I recommend to hold back tasks -
   * in line with the text above - iff
   *
   * - your tasking runtime struggles to handle many jobs. This happens with
   *   OpenMP for example as you have implicit scheudling points, i.e. OpenMP
   *   is allowed to process spawned tasks immediately. GNU does this one the
   *   number of tasks exceeds a certain threshold. You don't want this in
   *   Peano, so you can hold tasks back.
   * - you want to fuse tasks and offload them en block to the GPU. This is a
   *   reasonable motivation as kernel launches are extremely expensive. In
   *   this case, hold up to N tasks back if you fuse N tasks in one bash,
   *   but do not hold back any tasks if you are outside of a BSP section, as
   *   no new tasks will be created anymore and you want the tasks to be done.
   *
   * ## Realisation
   *
   * The routine is not const, as I want strategies give the opportunity to
   * adopt decisions after each call.
   *
   * ## Invocation pattern
   *
   * This routine is called once per task spawned (to know if we
   * maybe should immediately map it onto a native task), and then
   * at each end of the BSP section. When it is called for a particular
   * task, we pass in a proper task type. That is, the decision of the
   * strategy may depend on the type of the task for which we ask. At
   * the end of a BSP section, we pass in
   * tarch::multicore::orchestration::Strategy::EndOfBSPSection instead of a
   * particular task type.
   *
   * tarch::multicore::spawnAndWait() is the routine which triggers the query
   * for the end of a BSP section. If we have N tasks and N is bigger than the
   * result of this outine, it will map tasks onto native tasks through
   * internal::mapPendingTasksOntoNativeTasks().
   *
   * tarch::multicore::spawnTask() is the routine which queries this routine
   * for each and every task.
   *
   *
   * @param taskType Either actual task type if we get a task or
   *   EndOfBSPSection if it is not asked for a particular task
   *   type or, well, at the end of a fork-join part.
   */
  virtual int getNumberOfTasksToHoldBack(int taskType) = 0;

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
   * @param taskType Either actual task type if we get a task or
   *   EndOfBSPSection if it is not asked for a particular task
   *   type or, well, at the end of a fork-join part.
   */
  virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice(int taskType) = 0;

  /**
   * If you set this flag, tasks are immediately fused. You can ensure
   * that only idling threads fuse tasks. In this case, return false.
   */
  virtual bool fuseTasksImmediatelyWhenSpawned(int taskType) = 0;

  /**
   * Determine how to handle/realise parallelisation within fork/join region
   *
   * Peano models its execution with multiple parallel, nested fork/join
   * sections. You could also think of these as mini-BSP sections. This
   * routine guides the orchestration how to map those BSP sections onto
   * tasks.
   *
   * The decision can be guided by basically arbitrary contextual factors. The
   * most important one for me is the nesting factor. As we work mainly with
   * OpenMP, where tasks are tied to one core, it makes limited sense to have
   * nested parallel fors. Notably, it makes stuff slower. So usually, I return
   * ExecutionPolicy::RunSerially with anything with a nesting level greater
   * than 1.
   *
   * @param nestedParallelismLevel Please compare with tarch::multicore::spawnAndWait()
   *   which ensures that this flag equals 1 on the top level. A parameter of
   *   0 would mean that no fork/join region has been opened. For such a
   *   parameter, the code would not query this function.
   *
   * @param taskType We assume that all numberOfTasks tasks are of the same
   *   type.
   */
  virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) = 0;
};
