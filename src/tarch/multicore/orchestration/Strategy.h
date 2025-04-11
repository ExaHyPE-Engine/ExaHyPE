// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Strategy;

      /**
       * Default strategy
       *
       * The default strategy is AllOnGPU at the moment.
       */
      Strategy* createDefaultStrategy();
    } // namespace orchestration
  }   // namespace multicore
} // namespace tarch

#include <string>
#include <utility>
#include <chrono>


/**
 * Interface for any task orchestration
 *
 * There are multiple orchestration strategies implementing this interface and
 * hence guiding the task execution pattern. You can create those and make them
 * live by calling tarch::multicore::setOrchestration().
 *
 * Please consult the operation fuse() and its result type, FuseInstruction,
 * for details on the exact decision process.
 *
 */
class tarch::multicore::orchestration::Strategy {
public:
  /**
   * Provide hint of execution policy
   *
   * @see tarch::multicore::spawnAndWait() for rationale and details re the BSP
   *      sections of the code.
   * @see tarch::multicore::taskfusion for some remarks how the policies
   *      implemented through a strategy affect the task fusion.
   * @see The page @ref page_tarch_multicore for generic documentation on the
   *      design of the whole multicore component.
   */
  enum class ExecutionPolicy {
    RunSerially,
    RunParallel
  };

  /**
   * Fuse instruction
   *
   * Represent if and how to fuse tasks:
   *
   * @param device Which device is to be used. Can be
   *   tarch::multicore::Task::Host.
   * @param minTasks How many tasks are to be used at least. Can be zero, which
   *   means we always have to fuse (unless maxTask equals 1), but should not
   *   be negative.
   * @param maxTasks How many tasks are to be fused at most. Has to be bigger
   *   than minTasks. If you set it to one however, you disable fusion and
   *   tasks go straight into the runtime system as proper tasks.
   * @param timeoutNanoSeconds Timeout, i.e. time to wait for the right number
   *   of fused tasks to arrive.
   *
   * @see Consult the page @ref documentation_tarch_task_fusion_strategies
   *   for recommendations and hints on task fusion strategies.
   * @see Consult the namespace documentation of tarch::multicore::taskfusion
   *   for details on the realisation of the task fusion.
   */
  struct FuseInstruction {
    int device;
    int minTasks;
    int maxTasks;
    std::chrono::microseconds timeoutMicroSeconds;
    FuseInstruction(int device_, int minTasks_, int maxTasks_, const std::chrono::microseconds& timeoutMicroSeconds_ = std::chrono::microseconds(0)):
      device(device_),
      minTasks(minTasks_),
      maxTasks(maxTasks_),
      timeoutMicroSeconds(timeoutMicroSeconds_) {}

    std::string toString() const {
      return "(#device=" + std::to_string(device) + ",min=" + std::to_string(minTasks)
             + ",max=" + std::to_string(maxTasks) + ",timeout=" + std::to_string(timeoutMicroSeconds.count()) + "ms)";
    }
  };

  virtual ~Strategy() = default;

  /**
   * Notifies the strategy that we enter a BSP section
   */
  virtual void startBSPSection(int nestedParallelismLevel) = 0;

  /**
   * Notifies the strategy that we leave a BSP (fork-join) section
   */
  virtual void endBSPSection(int nestedParallelismLevel) = 0;
  
  virtual void updateFuseStatistics(
    int    taskType,
    int    queueSize,
    double averageNumberOfPendingReadyTasksPerProcessTaskExecution
  ) = 0;

  static constexpr int EndOfBSPSection = -1;

  /**
   * How many tasks to fuse and to which device to deploy
   *
   * @see Consult the page @ref documentation_tarch_task_fusion_strategies
   *   for recommendations and hints on task fusion strategies.
   * @see Consult the namespace documentation of tarch::multicore::taskfusion
   *   for details on the realisation of the task fusion.
   *
   * @param taskType Task type for which we want to know if to fuse or not.
   */
  virtual FuseInstruction fuse(int taskType) = 0;

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
   * @param taskType If we enter a fork-join section, this section logically
   *   spawns a set of tasks, which are all of the same type. So the task type
   *   here is given implicitly by the code location. But each BSP section has
   *   a unique identifier.
   */
  virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) = 0;
};
