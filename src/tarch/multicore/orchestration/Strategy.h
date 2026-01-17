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
       * The default strategy is AllOnGPU at the moment if you have GPU
       * support. Without GPU support, we use a Hardcoded variant that
       *
       */
      Strategy* createDefaultStrategy();

      /**
       * Control how the fusion is implemented
       *
       * We provide various fusion variants from which you can pick through the
       * tasking orchestration. Together with the executor chosen through the
       * task's fuse() operation (see discussion at @ref documentation_tarch_task_fusion_strategies),
       * we can construct a wide variety of different task execution schemes.
       *
       *
       * ## Remark on file/namespace organisation
       *
       * The fusion implementation is stores within the tarch::multicore::orchestration
       * namespace and therefore directory. This makes sense as it is up to the
       * orchestration strategy to pick the fusion of choice. At the same time,
       * it is inconsistent, as all the other fusion stuff is in a dedicated
       * namespace tarch::multicore::taskfusion. You hence might have to switch
       * forth and back between different namespaces and directories.
       *
       *
       * ## How and where this enum is used for decision making
       *
       * The enumeration is mainly used within the outward-facing routines
       * in tarch::multicore::taskfusion to decide which particular strategy to
       * invoke. That is, each strategy is implemented by providing different
       * flavours of three core routines. The user calls a blanco (interface)
       * variant of these routines, the routines query the active strategy,
       * and then delegate the function call to a particular implementation of
       * the routine.
       *
       * In C++, one might want to use an interface for this, but we decided to
       * stick to an applicative interface here.
       *
       *
       * ## Realisation of different task fusion strategies
       *
       * All task fusion realisations rely on the tarch::multicore::taskfusion::FusableTaskQueue:
       * the task queue offers a certain set up operations. The different
       * fusion strategies call/use these operations in different ways. It  is
       * a very small set of operations on the queue, which allow us to
       * construct various task process patterns for fusion.
       *
       *
       * ## Deprecated variants (lessons learned)
       *
       * Originally, I thought we'd need a couple of these
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * while (queue.second->isTaskReadyOrRunning(taskNumber, false)) {
       *   queue.second->processTasks();
       * }
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * whenever we have to ensure that a task is done. After all, someone else
       * might just have used this one as part of task fusion. However, such a
       * while is toxic. It can always lead to deadlocks. Instead, every task has
       * to be mapped onto a task dependency, and we have to ensure that we can
       * properly wait for tasks. It is fine if a task is already done, and in
       * this case we can prune the DAG and omit a wait, but otherwise, we have
       * to ensure that we work with task dependencies.
       *
       * So the punchline is: Polling does not work. Therefore, I don't offer
       * a strategy using polling.
       *
       *
       *
       * ## Offered variants
       *
       * Please see the list/comments below for variants that we do support
       * for Peano's tarch:
       *
       */
      enum class FusionImplementation {
        /**
         * This option effectively disables fusion as well and all fusable tasks are eagerly executed.
         *
         * Completely disable any fusion and map each task that can be fused on
         * a normal task due to a call to tarch::multicore::native::spawnTask() in
         * tarch::multicore::taskfusion::handleFusableTask().
         * The operation
         * tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks() deteriorates to
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
         * Created for debugging only, as it disables the fusion.
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
         * predecessing tasks is still pending in a fusable queue. If so, we
         * process it, but we also might combine it with multiple other
         * tasks.
         *
         * @image html src/tarch/multicore/taskfusion/taskfusion-LazyEvaluation.png
         *
         * After we know that they are either running or ready, we trigger the
         * fusion. The core routines to study how this is all implemented are
         *
         * - tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks_LazyEvaluation() which
         *   we use before we trigger a wait or handle a new task with incoming
         *   dependencies;
         * - tarch::multicore::taskfusion::handleFusableTask_LazyEvaluation() which
         *   does all the enqueuing.
         *
         *
         *
         * #### Implementation support
         *
         * This variant should work no matter what backend people use. It
         * reliably leads to large batches of tasks, as it is lazy and hence
         * waits for enough tasks to be collected. The disadvantage is that it
         * might trigger the fusion rather late.
         *
         * To understand this routine, it makes sense to study ProducerConsumerTaskSequence
         * and EagerConsumerTasks, too. Notably ProducerConsumerTaskSequence
         * provides context about important implementational considerations.
         *
         *
         */
        LazyEvaluation,

        /**
         * Timed execution
         *
         * Timed execution really means lazy evaluation where all tasks are
         * fused "manually" at a point which is determined by the user.
         *
         * There's no need for a dedicated timed execution variant:
         * You can use the lazy task fusion strategy to collect fusable tasks
         * and to handle then at one particular point in time, i.e. to issue
         * the fusion manually. To do so, you have to invoke
         * tarch::multicore::taskfusion::processAndFuseAllReadyTasks()
         * manually.
         *
         * However, there is no built-in version for the timed execution, as
         * the technical architecture cannot and should not know about the
         * right point to issue the fusion. Therefore, we provide this option
         * which is internally exactly the same as LazyEvaluation. However,
         * using this flag, your main can determine if it wants to manually
         * trigger
         * tarch::multicore::taskfusion::processAndFuseAllReadyTasks()
         * at a certain point. ExaHyPE for example does so.
         */
        TimedExecution,

        /**
         * Map each fusable task onto a producer-consumer task pair
         *
         * Whenever we get a task (in the example below we get three yellow
         * tasks) with dependencies that can be fused, we split this task up
         * into three logical tasks:
         *
         * @image html src/tarch/multicore/taskfusion/taskfusion-ProducerConsumerTaskSequence.png
         *
         * One task is a preparation task (red), and the other one is the
         * actual processing (yellow). In-between there's a fusion task (blue).
         *
         * If we spawn a task without in-dependencies, i.e. a ready task, we
         * deposit this task in a separate queue (an instance of FusableTaskQueue).
         * If there in in-dependencies, we create a LogReadyTask instance with
         * those in-dependencies, which takes the actual task and deposits it
         * in the queue once it becomes ready.
         *
         * The fusion task comes next and grabs an arbitrary number of tasks
         * to be fused. Also, it marks those tasks within FusableTaskQueue
         * and leaves its task number. We need this to avoid deadlocks.
         *
         * The process task finally takes the particular task from the queue
         * and handles it (unless it is done already).
          *
         * <div style="background-color: #fcc ; padding: 10px; border: 1px solid green;">
         * This variant might require OpenMP 6.
         * </div>
         *
         *
         * <div style="background-color: #cfc ; padding: 10px; border: 1px solid green;">
         * This variant tripples the number of tasks pending in the system and hence
         * has the potential to lead to out-of-memory situations.
         * </div>
         *
         * #### Related strategies
         *
         * EagerConsumerTasks is similar but does not spawn the final process
         * task yet. This is only spawned once we need it. Further to that, it
         * spawns ProcessSetOfReadyTasks, i.e. the fusion, if and only if there
         * are already enough ready tasks.
         *
         * LazyEvaluation moves this whole task sequence (besides the log ready
         * task part) into a lazy evaluation.
         *
         * @see tarch::multicore::taskfusion::LogReadyTask
         * @see tarch::multicore::taskfusion::ProcessSetOfReadyTasks
         */
        ProducerConsumerTaskSequence,

        /**
         * Issue consumer task that fuses upon spawning a task if the queue is big enough
         *
         * The idea here is that we enqueue any fusable task into our internal
         * queue and then immediately check how many tasks we have pending in
         * this queue. If the queue is big enough, i.e. holds more tasks than
         * the lower threshold of the strategy, then we issue a consumer task.
         *
         * @image html src/tarch/multicore/taskfusion/taskfusion-EagerConsumerTasks.png
         *
         *
         * The arising task pattern can be similar to LazyEvaluation, but it
         * turns out to be significantly different: Lazy spawning of fusion
         * tasks means that the fusion drops in later. That can be bad
         * for schemes which offload to GPUs, e.g., and hence need a large
         * overlap. This scheme avoids this penalty, as we issue fusion tasks
         * if and only they are ready to fly.
         *
         * From an implementation point of view, the variant can be seen as a
         * hybrid between lazy evaluation and producer-consumer tasks.
         * By default, we enqueue all tasks into the respective task queues.
         * This is the same strategy we followed for the lazy schemes.
         * However, we also check how many tasks are already in there and if
         * this number if big enough, we spawn one of our fusion tasks.
         * Different to the ProducerConsumerTaskSequence the fusion is not built
         * into the DAG right from the start, but the corresopnding task is
         * issued upon demand.
         *
         * <div style="background-color: #fcc ; padding: 10px; border: 1px solid green;">
         * This variant might require OpenMP 6.
         * </div>
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
   *   equal minTasks. If you set it to one, you disable fusion and
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
   * @param averageNumberOfReadyTasksPerProcessTaskExecution average
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
    double averageNumberOfReadyTasksPerProcessTaskExecution,
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
