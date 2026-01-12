// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/orchestration/Strategy.h"

namespace benchmarks {
  namespace exahype2 {
    namespace ccz4 {
      class MulticoreOrchestration;
    }
  }
}

/**
 * Hard coded strategy for the single black hole setup
 *
 * The single black hole setup is fixed setup, where we have a large domain
 * covered by higher order patches plus a small area in the centre which is
 * covered by Finite Volumes. The latter are very compute heavy and hence
 * quickly become the bottleneck. So we have to process them as soon as
 * possible. We could realise this by giving those tasks a higher priority
 * than all other tasks, but I actually prefer to realise all scheduling
 * within this orchestration object. Certainly, priorities might do a fine
 * job as well.
 *
 * ## Behaviour
 *
 * - The mesh can be rather complicated with AMR, and the inclusion of
 *   Finite Volume outcomes hence might be expensive. So we want to have
 *   all enclave tasks computed by the end of the first sweep if possible.
 * - Each FV patch makes an object well-suited for a GPU. If there is an
 *   accelerator, we offload immediately.
 * - If there is accelerator, we map each FV patch immediately to a
 *   proper GPU task.
 * - All other tasks are fused properly.
 * - We disable any nested parallelism. See paralleliseForkJoinSection()'s
 *   documentation.
 *
 */
class benchmarks::exahype2::ccz4::MulticoreOrchestration: public tarch::multicore::orchestration::Strategy {
  private:
    /**
     * Number of nested fork/join levels. Important for
     * paralleliseForkJoinSection() to decide if the parallel region should
     * actually be processed concurrently.
     */
    int _nestedBSPLevels;

    /**
     * Maximum number of finite volume tasks in the system. I don't use this
     * value at the moment, but might want to use it for GPUs.
     *
     * @see _finiteVolumeTasksInThisBSPSection
     */
    int _maxFiniteVolumeTasks;

    /**
     * Current number of finite volume tasks that already have been spawned.
     *
     * This is a local temporary attribute which I use to accumulate a
     * new value for _maxFiniteVolumeTasks, i.e. eventually this value
     * is rolled over into _maxFiniteVolumeTasks and cleared. While I
     * accumulate a new value, the "valid" attribute _maxFiniteVolumeTasks
     * remains unaltered, i.e. we work with the previous valid value.
     *
     * @see startBSPSection()
     */
    int _finiteVolumeTasksInThisBSPSection;

  public:
    MulticoreOrchestration();
    virtual ~MulticoreOrchestration() = default;

    /**
     * Start a fork/join section
     *
     * Reset _finiteVolumeTasksInThisBSPSection is this is the start of the
     * outermost parallel region.
     *
     * @see paralleliseForkJoinSection() which uses the nested parallelism
     *   counter.
     */
    virtual void            startBSPSection(int nestedParallelismLevel) override;

    /**
     * End fork/join section
     *
     * Decrement the counter _nestedBSPLevels. If the outermost parallel
     * region joins, we can update _maxFiniteVolumeTasks.
     *
     * @see paralleliseForkJoinSection() which uses the nested parallelism
     *   counter.
     */
    virtual void            endBSPSection(int nestedParallelismLevel) override;

    /**
     * Ensure right cardinality ends up on GPU
     *
     * If we hae a Finite Volume task, we send it off to the GPU immediately.
     * If we have FD4 tasks, we wait until we have 16 of them and then send them
     * off. The 16 is arbitrary. I just needed one number.
     */
    virtual FuseInstruction fuse(int taskType) override;

    /**
     * Determine how to parallelise a fork/join section
     *
     * I found nested parallelism to be brutally slow, so I always return
     * tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially
     * if more than one parallel level is embedded into each other. Otherwise,
     * I'm happy for a section to be processed in parallel.
     */
    virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) override;
};
