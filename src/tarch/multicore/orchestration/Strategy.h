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

      /**
       * Control how the fusion is implemented
       */
      enum class FusionImplementation {
        /**
         * This option effectively disables fusion as well and all fusable tasks are eagerly executed.
         *
         * Completely disable any fusion and map each task that can be fused on
         * a normal task due to a call to native::spawnTask() in
         * translateFusableTaskIntoTaskSequence().
         * ensureHeldBackTasksAreMappedOntoNativeTasks() deteriorates to
         * nop.
         *
         * <div style="background-color: #fcc ; padding: 10px; border: 1px solid green;">
         * This variant might require OpenMP 6, as the tasks might have transparent incoming or outgoing dependencies.
         * </div>
         *
         * Mainly there for debugging reasons.
         */
        DisableFusionAndMapOntoNativeTask,

        /**
         * Disable the fusion and process each fusable task immediately
         *
         * Supported for all backends. Created for debugging only, as it disables the fusion.
         *
         * <div style="background-color: #fcc ; padding: 10px; border: 1px solid green;">
         * This variant might require OpenMP 6, as the tasks might have transparent incoming or outgoing dependencies.
         * </div>
         *
         */
        DisableFusionAndProcessImmediately,

        /**
         * All fusable tasks are parked in a big queue and we fuse lazily as we need the outcome
         *
         * All fusable tasks are enqueued into a separate queue. When a new
         * task with in-dependencies is launched, we doublecheck if the
         * predecessing tasks is still pending in a fusable queue. This is done
         * in the parent namespace and leads to a call of
         * ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation().
         *
         * ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation() first
         * checks if the predecessor tasks are at least ready or running. If
         * they are not yet in that state, i.e. are still pending as their
         * preconditions in turn are not fulfilled yet, we wait for them. This
         * is a synchronisation point.
         *
         * After we know that they are either running or ready, we trigger the
         * fusion. The core routines to study how this is all implemented are
         * tarch::multicore::taskfusion::translateFusableTaskIntoTaskSequence_LazyEvaluation()
         * and tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation().
         *
         * This variant should work no matter what backend people use. It
         * reliably leads to large batches of tasks, as it is lazy and hence
         * waits for enough tasks to be collected. The disadvantage is that it
         * might trigger the fusion rather late.
         *
         */
        LazyEvaluation,

        /**
         * Map each fusable task onto a producer-consumer task pair
         *
         * Whenever we get a task (in the example below we get three yellow
         * tasks) with dependencies that can be fused, we split each task up
         * into two tasks each:
         *
         * @image html multicore_task_flow-00.png
         *
         * One task is a preparation task (red), and the other one is the
         * actual processing (green). The prep task has all the complex
         * incoming dependencies. The processing task is part of a straight
         * task sequence. Both the prep task and the processing task know the
         * actual task, which is not part of the actual task graph anymore. It
         * is associated with these two wrapper classes.
         *
         * @image html multicore_task_flow-01.png
         *
         * If the prep task becomes active, we do not evaluate the task.
         * Instead, we dump the task into the queue of ready tasks.
         *
         * @image html multicore_task_flow-02.png
         * @image html multicore_task_flow-03.png
         *
         * The preparation is done through tarch::multicore::taskfusion::LogReadyTask,
         * which is rather trivial. The actual task execution and fusion is
         * then realised through and instance of ProcessReadyTask which holds
         * also all race condition logic. See the class documentation. This
         * processing task finally also serves as placeholder for outgoing
         * dependencies.
         *
         * <div style="background-color: #fcc ; padding: 10px; border: 1px solid green;">
         * This variant might require OpenMP 6.
         * </div>
         *
         *
         */
        ProducerConsumerTaskPairs,

        /**
         * Issue consumer task that fuses upon spawning a task if the queue is big enough
         *
         * The idea here is that we enqueue any fusable task into our internal
         * queue and then immediately check how many tasks we have pending in
         * this queue. If the queue is big enough, i.e. holds more tasks than
         * the lower threshold of the strategy, then we issue a consumer task.
         *
         * The arising task pattern can be similar to LazyConsumerTasks, but it
         * turns out to be significantly different: Lazy invocation of consumer
         * tasks means that the consumer tasks drops in later. That can be bad
         * for schemes which offload to GPUs, e.g., and hence need a large
         * overlap.
         *
This is a total hybrid between lazy evaluation and producer-consumer tasks.
By default, we enqueue all tasks into the respective task queues.
This is the same strategy we followed for the lazy schemes.
However, we also check how many tasks are already in there and if this number if big enough, we spawn one of our consumer tasks.
So consumer tasks can either be spawned lazily when we realise that we need the outcome of a fusable tasks - this is the same implementation as for LazyConsumerTasks - or they can be issued early as we have enough fusable tasks pending already.


<div style="background-color: #fcc ; padding: 10px; border: 1px solid green;">
This variant might require OpenMP 6.
</div>
         */
        EagerConsumerTasks
      };
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
    FuseInstruction(int device_, int minTasks_, int maxTasks_):
      device(device_),
      minTasks(minTasks_),
      maxTasks(maxTasks_)
      {}

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
   * Notifies the strategy that we leave a BSP (fork-join) section
   */
  virtual void endBSPSection(int nestedParallelismLevel) = 0;
  
  /**
   * Update task fusion status
   *
   * Feed orchestration with some stats around the task queue of interest.
   * This routine is not called every time a process read tasks kicks off,
   * but only every now and then. Furthermore,
   * please note that this routine is invoked iff the orchestration beforehand
   * had decided to fuse tasks.
   *
   * @param taskType Number of the task that we are studying
   * @param queueSize Size of the queue at this observation point
   * @param averageNumberOfPendingReadyTasksPerProcessTaskExecution average
   *   over number of tasks in the queue whenever a process task has kicked
   *   off and the underlying real task has not completed yet.
   * @param probabilityThatTaskIsCompleted Probability that task result has
   *   been there already when the corresponding instance of ProcessReadyTask
   *   fires up. This is a biased value: If we try to look up if a task is
   *   already completed and the underlying dictionary is locked, then this
   *   counts as a "no hit" as well.
   */
  virtual void updateFuseStatistics(
    int    taskType,
    int    queueSize,
    double averageNumberOfPendingReadyTasksPerProcessTaskExecution,
    double probabilityThatTaskIsCompleted
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

  /**
   * Determine how the implementation handles fusable tasks
   *
   * In theory, you can change the strategy at runtime. However, such a
   * switching is not well-defined and might lead to errors.
   */
  virtual FusionImplementation getFusionImplementation() const = 0;
};
